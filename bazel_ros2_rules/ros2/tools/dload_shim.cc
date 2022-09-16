#include "dload_shim.h"

#include <string.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;

int do_dload_shim(
  const int argc,
  const char ** argv,
  const char * executable_path,
  const std::vector<const char *> names,
  const std::vector<std::vector<const char *>> actions)
{
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(argv[0], &error));
  if (!runfiles && std::filesystem::is_symlink(argv[0])) {
    runfiles.reset(
      Runfiles::Create(std::filesystem::read_symlink(argv[0]), &error));
  }
  if (!runfiles) {
    std::cerr << "DLOAD SHIM ERROR: " << error << std::endl;
    return -1;
  }

  // Forward runfiles env vars if needed. This is necessary since our shims are
  // (presently) separate C++ binaries, and inferring runfiles manifests via
  // `argv[0]` will not work properly when run outside of Bazel (#105).
  // This will check if any runfiles env vars are set; if so, we assume this is
  // a nested invocation and will not overwrite the env vars, and use the
  // existing runfiles that are set.
  const auto& runfiles_env = runfiles->EnvVars();
  bool has_exiting_runfiles_env = false;
  for (const auto& [key, value] : runfiles_env) {
    if (nullptr != getenv(key.c_str())) {
      has_exiting_runfiles_env = true;
      break;
    }
  }
  if (!has_exiting_runfiles_env) {
    for (const auto& [key, value] : runfiles_env) {
      if (setenv(key.c_str(), value.c_str(), 1) != 0) {
        std::cerr << "DLOAD SHIM ERROR: failed to set " << key << std::endl;
      }
    }
  }

  // Sentinel indicates if the executable has already been shimmed
  const char * kShimmedSentinel = "_BAZEL_ROS2_RULES_SHIMMED";

  // Actions indicate how to change environment variables
  const std::string kReplace = "replace";
  const std::string kSetIfNotSet = "set-if-not-set";
  const std::string kPathPrepend = "path-prepend";
  const std::string kPathReplace = "path-replace";

  if (nullptr == getenv(kShimmedSentinel)) {
    // Apply actions.
    for (size_t i = 0; i < names.size(); ++i) {
      std::stringstream value_stream;
      if (actions[i][0] == kReplace) {
        if (actions[i].size() != 2) {
          std::abort();
        }
        value_stream << actions[i][1];
      } else if (actions[i][0] == kSetIfNotSet) {
        if (actions[i].size() != 2) {
          std::abort();
        }
        if (nullptr != getenv(names[i])) {
          continue;
        }
        value_stream << actions[i][1];
      } else if (actions[i][0] == kPathReplace) {
        if (actions[i].size() != 2) {
          std::abort();
        }
        value_stream << runfiles->Rlocation(actions[i][1]);
      } else if (actions[i][0] == kPathPrepend) {
        if (actions[i].size() < 2) {
          std::abort();
        }
        for (size_t j = 1; j < actions[i].size(); ++j) {
          value_stream << runfiles->Rlocation(actions[i][j]) << ":";
        }

        const char * raw_value = getenv(names[i]);
        if (raw_value != nullptr) {
          value_stream << raw_value;
        }
      } else {
        std::abort();  // should never get here
      }
      std::string value = value_stream.str();

      std::string::size_type location;
      if ((location = value.find("$PWD")) != std::string::npos) {
        value.replace(location, 4, std::filesystem::current_path());
      }

      if (setenv(names[i], value.c_str(), 1) != 0) {
        std::cerr << "DLOAD SHIM ERROR: failed to set " << names[i] << "\n";
      }
    }
    setenv(kShimmedSentinel, "", 1);
  }

  const std::string real_executable_path = runfiles->Rlocation(executable_path);

  char ** other_argv = new char*[argc + 1];
  other_argv[0] = strdup(real_executable_path.c_str());
  for (int i = 1; i < argc; ++i) {
    other_argv[i] = strdup(argv[i]);
  }
  other_argv[argc] = nullptr;
  int ret = execv(other_argv[0], other_argv);
  // What follows applies if and only if execv() itself fails
  // (e.g. can't find the binary) and returns control
  std::cout << "DLOAD SHIM ERROR: " << strerror(errno) << std::endl;
  return ret;
}
