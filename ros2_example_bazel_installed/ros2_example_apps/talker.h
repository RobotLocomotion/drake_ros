#include <chrono>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>


class Talker : public rclcpp::Node {
 public:
  Talker() : rclcpp::Node("talker"){
    publisher_ = this->create_publisher<std_msgs::msg::String>("chatter", 10);
    timer_ = this->create_wall_timer(
        std::chrono::duration<double>(0.1),
        std::bind(&Talker::timer_callback, this));
  }

 private:
  void timer_callback() const {
    std_msgs::msg::String message;
    message.data = "Hello from Talker";
    publisher_->publish(message);
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};
