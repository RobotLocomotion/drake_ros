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

#include "drake_ros_core/drake_ros.h"

#include <chrono>
#include <memory>
#include <string>

#include <rclcpp/executors.hpp>
#include <rclcpp/node.hpp>

namespace drake_ros_core {
struct DrakeRos::Impl {
  rclcpp::Context::SharedPtr context;
  rclcpp::Node::UniquePtr node;
  rclcpp::executors::SingleThreadedExecutor::UniquePtr executor;
};

DrakeRos::DrakeRos(const std::string& node_name,
                   rclcpp::NodeOptions node_options)
    : impl_(new Impl()) {
  if (!node_options.context()) {
    // Require context is constructed (NodeOptions uses Global Context by
    // default)
    throw std::invalid_argument("NodeOptions must contain a non-null context");
  }
  impl_->context = node_options.context();

  impl_->node.reset(new rclcpp::Node(node_name, node_options));

  // TODO(hidmic): optionally take a user-provided Executor instance
  rclcpp::ExecutorOptions eo;
  eo.context = impl_->context;
  impl_->executor.reset(new rclcpp::executors::SingleThreadedExecutor(eo));

  impl_->executor->add_node(impl_->node->get_node_base_interface());
}

DrakeRos::~DrakeRos() {}

const rclcpp::Node& DrakeRos::get_node() const { return *impl_->node; }

rclcpp::Node* DrakeRos::get_mutable_node() const { return impl_->node.get(); }

void DrakeRos::Spin(int timeout_millis) {
  if (timeout_millis < 0) {
    // To match `DrakeLcm::HandleSubscriptions()`'s behavior,
    // throw if timeout is negative.
    throw std::runtime_error("timeout cannot be negative");
  }
  // TODO(hidmic): switch to rclcpp::Executor::spin_all() when and if a zero
  // timeout is supported. See https://github.com/ros2/rclcpp/issues/1825.
  impl_->executor->spin_some(std::chrono::milliseconds(timeout_millis));
}

void init(int argc, const char** argv) {
  if (!rclcpp::ok()) {
    rclcpp::init(argc, argv);
  }
}

bool shutdown() { return rclcpp::shutdown(); }

}  // namespace drake_ros_core
