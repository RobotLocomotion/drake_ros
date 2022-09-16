#include <chrono>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include "rmw_isolation/rmw_isolation.h"

using std::placeholders::_1;

class IsolatedListener : public rclcpp::Node {
 public:
  IsolatedListener(const std::string& uuid)
      : rclcpp::Node("isolated_listener"), uuid_(uuid) {
    double timeout = this->declare_parameter("timeout", 2.0);
    subscription_ = this->create_subscription<std_msgs::msg::String>(
        "uuid", 10, std::bind(&IsolatedListener::topic_callback, this, _1));
    timer_ = this->create_wall_timer(
        std::chrono::duration<double>(timeout),
        std::bind(&IsolatedListener::timer_callback, this));
  }

 private:
  void topic_callback(const std_msgs::msg::String::SharedPtr msg) {
    if (msg->data != uuid_) {
      throw std::runtime_error("I heard '" + msg->data + "' yet " +
                               "I was expecting '" + uuid_ + "'!");
    }
    ++expected_messages_count_;
  }

  void timer_callback() const {
    if (0u == expected_messages_count_) {
      throw std::runtime_error("I did not hear '" + uuid_ + "' even once!");
    }
    rclcpp::shutdown();
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr timer_;
  size_t expected_messages_count_{0u};
  std::string uuid_;
};

int main(int argc, char* argv[]) {
  const char* TEST_TMPDIR = std::getenv("TEST_TMPDIR");
  if (TEST_TMPDIR != nullptr) {
    ros2::isolate_rmw_by_path(argv[0], TEST_TMPDIR);
  }
  rclcpp::init(argc, argv);
  std::string uuid{TEST_TMPDIR != nullptr ? TEST_TMPDIR : "none"};
  rclcpp::spin(std::make_shared<IsolatedListener>(uuid));
  rclcpp::shutdown();
  return 0;
}
