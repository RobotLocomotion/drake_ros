#include <cstdlib>
#include <iostream>

const char * kShimmedSentinel = "_BAZEL_ROS2_RULES_SHIMMED";

int main(int argc, char* argv[]) {
  std::cout << "shimmed: ";
  if (nullptr != getenv(kShimmedSentinel)) {
    std::cout << "yes";
  } else {
    std::cout << "no";
  }

  std::cout << " AMENT_PREFIX_PATH present: ";
  if (nullptr != getenv("AMENT_PREFIX_PATH")) {
    std::cout << "yes";
  } else {
    std::cout << "no";
  }
  return 0;
}
