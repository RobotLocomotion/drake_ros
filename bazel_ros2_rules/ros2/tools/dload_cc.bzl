# -*- python -*-

"""
The purpose of these rules is to support the propagation of runtime information
that is necessary for the execution of C/C++ binaries and tests that require it.
"""

load(
    "//tools:dload.bzl",
    "do_dload_shim",
    "get_dload_shim_attributes",
)

# TODO(eric.cousineau): We should ideally separate out as much as this actual
# logic into separate static library, and make entry point minimal.
_DLOAD_CC_SHIM_TEMPLATE = """\
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;

int main(int argc, const char * argv[]) {{
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(argv[0], &error));
  if (!runfiles && std::filesystem::is_symlink(argv[0])) {{
    runfiles.reset(
      Runfiles::Create(std::filesystem::read_symlink(argv[0]), &error));
  }}
  if (!runfiles) {{
    std::cerr << "DLOAD SHIM ERROR: " << error << std::endl;
    return -1;
  }}

  // Forward runfiles env vars if needed. This is necessary since our shims are
  // (presently) separate C++ binaries, and inferring runfiles manifests via
  // `argv[0]` will not work properly when run outside of Bazel (#105).
  // This will check if any runfiles env vars are set; if so, we assume this is
  // a nested invocation and will not overwrite the env vars, and use the
  // existing runfiles that are set.
  const auto& runfiles_env = runfiles->EnvVars();
  bool has_exiting_runfiles_env = false;
  for (const auto& [key, value] : runfiles_env) {{
    if (nullptr != getenv(key.c_str())) {{
      has_exiting_runfiles_env = true;
      break;
    }}
  }}
  if (!has_exiting_runfiles_env) {{
    for (const auto& [key, value] : runfiles_env) {{
      if (setenv(key.c_str(), value.c_str(), 1) != 0) {{
        std::cerr << "DLOAD SHIM ERROR: failed to set " << key << std::endl;
      }}
    }}
  }}

  // Sentinel indicates if the executable has already been shimmed
  const char * kShimmedSentinel = "_BAZEL_ROS2_RULES_SHIMMED";

  if (nullptr == getenv(kShimmedSentinel)) {{
    // Apply actions.
    std::vector<std::string> names = {names};
    std::vector<std::vector<std::string>> actions = {actions};  // NOLINT
    for (size_t i = 0; i < names.size(); ++i) {{
      std::stringstream value_stream;
      if (actions[i][0] == "replace") {{
        assert(actions[i].size() == 2);
        value_stream << actions[i][1];
      }} else if (actions[i][0] == "set-if-not-set") {{
        assert(actions[i].size() == 2);
        if (nullptr != getenv(names[i].c_str())) {{
          continue;
        }}
        value_stream << actions[i][1];
      }} else if (actions[i][0] == "path-replace") {{
        assert(actions[i].size() == 2);
        value_stream << runfiles->Rlocation(actions[i][1]);
      }} else if (actions[i][0] == "path-prepend") {{
        assert(actions[i].size() >= 2);
        for (size_t j = 1; j < actions[i].size(); ++j) {{
          value_stream << runfiles->Rlocation(actions[i][j]) << ":";
        }}

        const char * raw_value = getenv(names[i].c_str());
        if (raw_value != nullptr) {{
          value_stream << raw_value;
        }}
      }} else {{
        assert(false);  // should never get here
      }}
      std::string value = value_stream.str();

      std::string::size_type location;
      if ((location = value.find("$PWD")) != std::string::npos) {{
        value.replace(location, 4, std::filesystem::current_path());
      }}

      if (setenv(names[i].c_str(), value.c_str(), 1) != 0) {{
        std::cerr << "DLOAD SHIM ERROR: failed to set " << names[i] << std::endl;
      }}
    }}
    setenv(kShimmedSentinel, "", 1);
  }}

  const std::string executable_path =
      runfiles->Rlocation("{executable_path}");  // NOLINT

  char ** other_argv = new char*[argc + 1];
  other_argv[0] = strdup(executable_path.c_str());
  for (int i = 1; i < argc; ++i) {{
    other_argv[i] = strdup(argv[i]);
  }}
  other_argv[argc] = nullptr;
  int ret = execv(other_argv[0], other_argv);
  // What follows applies if and only if execv() itself fails
  // (e.g. can't find the binary) and returns control
  std::cout << "DLOAD SHIM ERROR: " << strerror(errno) << std::endl;
  return ret;
}}
"""

def _to_cc_list(collection):
    """Turn collection into a C++ aggregate initializer expression."""
    return "{" + ", ".join(collection) + "}"

def _dload_cc_shim_impl(ctx):
    return do_dload_shim(ctx, _DLOAD_CC_SHIM_TEMPLATE, _to_cc_list)

dload_cc_shim = rule(
    attrs = get_dload_shim_attributes(),
    implementation = _dload_cc_shim_impl,
    output_to_genfiles = True,
)
"""
Generates a C++ shim that can inject runtime environment information for
C/C++ binaries that have such requirements. Using a C++ shim for C++ binaries
simplifies UX during debugging sessions, as fork-follow behavior in common
debuggers like gdb and lldb makes it transparent.

See do_dload_shim() documentation for further reference.
"""
