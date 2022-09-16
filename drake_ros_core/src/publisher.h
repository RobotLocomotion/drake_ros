// Copyright 2021 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <memory>
#include <string>

#include <rclcpp/publisher_base.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/serialized_message.hpp>
#include <rosidl_runtime_c/message_type_support_struct.h>

namespace drake_ros_core {
namespace internal {
// A type-erased version of rclcpp:::Publisher<Message>.
// This class conforms to the ROS 2 C++ style for consistency.
class Publisher final : public rclcpp::PublisherBase {
 public:
  Publisher(rclcpp::node_interfaces::NodeBaseInterface* node_base,
            const rosidl_message_type_support_t& ts,
            const std::string& topic_name, const rclcpp::QoS& qos);

  ~Publisher();

  void publish(const rclcpp::SerializedMessage& serialized_msg);
};
}  // namespace internal
}  // namespace drake_ros_core
