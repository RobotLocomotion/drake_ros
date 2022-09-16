#include <chrono>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include "rmw_isolation/rmw_isolation.h"

#include "listener.h"
#include "talker.h"


int main(int argc, char* argv[]) {
  const char* TEST_TMPDIR = std::getenv("TEST_TMPDIR");
  if (TEST_TMPDIR != nullptr) {
    std::string ros_home = std::string(TEST_TMPDIR) + "/.ros";
    setenv("ROS_HOME", ros_home.c_str(), 1);
    ros2::isolate_rmw_by_path(argv[0], TEST_TMPDIR);
  }
  rclcpp::init(argc, argv);

  auto talker = std::make_shared<Talker>();
  auto listener = std::make_shared<Listener>();

  rclcpp::executors::SingleThreadedExecutor exec;
  exec.add_node(talker);
  exec.add_node(listener);

  auto got_message = listener->NextMessage();

  auto result =
    exec.spin_until_future_complete(got_message, std::chrono::seconds(5));

  rclcpp::shutdown();

  if (result == rclcpp::FutureReturnCode::SUCCESS) {
    return 0;
  }
  return 1;
}
