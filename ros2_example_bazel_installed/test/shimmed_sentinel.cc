#include <cstdlib>
#include <iostream>

const char * kShimmedSentinel = "_BAZEL_ROS2_RULES_SHIMMED";

int main(int argc, char* argv[]) {
  bool shimmed = (nullptr != getenv(kShimmedSentinel));
  bool app_present = (nullptr != getenv("AMENT_PREFIX_PATH"));

  std::cout << "{\"shimmed\": " << shimmed
    << ", \"AMENT_PREFIX_PATH present\": " << app_present << "}";
  return 0;
}
