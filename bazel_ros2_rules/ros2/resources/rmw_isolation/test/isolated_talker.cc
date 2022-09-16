#include <chrono>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include "rmw_isolation/rmw_isolation.h"

class IsolatedTalker : public rclcpp::Node {
 public:
  IsolatedTalker(const std::string& uuid)
      : rclcpp::Node("isolated_talker"), uuid_(uuid) {
    publisher_ = this->create_publisher<std_msgs::msg::String>("uuid", 10);
    timer_ = this->create_wall_timer(
        std::chrono::duration<double>(0.1),
        std::bind(&IsolatedTalker::timer_callback, this));
  }

 private:
  void timer_callback() const {
    std_msgs::msg::String message;
    message.data = uuid_;
    publisher_->publish(message);
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::string uuid_;
};

int main(int argc, char* argv[]) {
  const char* TEST_TMPDIR = std::getenv("TEST_TMPDIR");
  if (TEST_TMPDIR != nullptr) {
    ros2::isolate_rmw_by_path(argv[0], TEST_TMPDIR);
  }
  rclcpp::init(argc, argv);
  std::string uuid{TEST_TMPDIR != nullptr ? TEST_TMPDIR : "none"};
  rclcpp::spin(std::make_shared<IsolatedTalker>(uuid));
  rclcpp::shutdown();
  return 0;
}
