// See `runfiles_direct_test.sh` for motivation.
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;

// Simplified version of
// https://github.com/RobotLocomotion/drake/blob/v1.5.0/common/find_runfiles.cc#L36-L123
std::unique_ptr<Runfiles> CreateRunfiles() {
  std::unique_ptr<Runfiles> runfiles;
  std::string bazel_error;
  if (std::getenv("TEST_SRCDIR")) {
    runfiles.reset(Runfiles::CreateForTest(&bazel_error));
  } else if ((std::getenv("RUNFILES_MANIFEST_FILE") != nullptr) ||
             (std::getenv("RUNFILES_DIR") != nullptr)) {
    runfiles.reset(Runfiles::Create({}, &bazel_error));
  } else {
    const std::string& argv0 = std::filesystem::read_symlink({
        "/proc/self/exe"}).string();
    runfiles.reset(Runfiles::Create(argv0, &bazel_error));
  }
  if (runfiles == nullptr) {
    throw std::runtime_error(bazel_error);
  }
  return runfiles;
}

int main() {
  auto runfiles = CreateRunfiles();
  std::string file_path = runfiles->Rlocation(
      "ros2_example_bazel_installed/test/runfiles_test_data.txt");
  std::cout << file_path << std::endl << "Good!" << std::endl;
  return 0;
}
