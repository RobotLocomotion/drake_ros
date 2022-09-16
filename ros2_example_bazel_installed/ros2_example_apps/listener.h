#include <chrono>
#include <future>
#include <memory>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

using std::placeholders::_1;

class Listener : public rclcpp::Node {
 public:
  Listener() : rclcpp::Node("listener") {
    subscription_ = this->create_subscription<std_msgs::msg::String>(
        "chatter", 10, std::bind(&Listener::topic_callback, this, _1));
  }

  std::future<std_msgs::msg::String::SharedPtr>
  NextMessage()
  {
    promises_.emplace_back();
    return promises_.back().get_future();
  }

 private:
  void topic_callback(const std_msgs::msg::String::SharedPtr msg) {
    for (auto & promise : promises_) {
      promise.set_value(msg);
    }
    promises_.clear();
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;

  std::vector<std::promise<std_msgs::msg::String::SharedPtr>> promises_;
};
