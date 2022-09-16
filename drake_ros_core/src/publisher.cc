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

#include "publisher.h"  // NOLINT(build/include)

#include <string>

namespace drake_ros_core {
namespace internal {
namespace {
// Copied from rosbag2_transport rosbag2_get_publisher_options
rcl_publisher_options_t publisher_options(const rclcpp::QoS& qos) {
  auto options = rcl_publisher_get_default_options();
  options.qos = qos.get_rmw_qos_profile();
  return options;
}
}  // namespace

Publisher::Publisher(rclcpp::node_interfaces::NodeBaseInterface* node_base,
                     const rosidl_message_type_support_t& type_support,
                     const std::string& topic_name, const rclcpp::QoS& qos)
    : rclcpp::PublisherBase(node_base, topic_name, type_support,
                            publisher_options(qos)) {}

Publisher::~Publisher() {}

void Publisher::publish(const rclcpp::SerializedMessage& serialized_msg) {
  // TODO(sloretz) Copied from rosbag2_transport GenericPublisher, can it be
  // upstreamed to rclcpp?
  auto return_code = rcl_publish_serialized_message(
      get_publisher_handle().get(),
      &serialized_msg.get_rcl_serialized_message(), NULL);

  if (return_code != RCL_RET_OK) {
    rclcpp::exceptions::throw_from_rcl_error(
        return_code, "failed to publish serialized message");
  }
}
}  // namespace internal
}  // namespace drake_ros_core
