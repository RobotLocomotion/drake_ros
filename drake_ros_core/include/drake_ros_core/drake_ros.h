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

#include <rclcpp/node.hpp>
#include <rclcpp/node_options.hpp>

namespace drake_ros_core {

/** A Drake ROS interface that wraps a live ROS node.

 This interface manages both ROS node construction and scheduling
 (using a `rclcpp::executors::SingleThreadedExecutor` instance).
 See `rclcpp::Node` and `rclcpp::Executor` documentation for further
 reference on expected behavior.
 */
class DrakeRos final {
 public:
  /** A constructor that wraps a `node_name` ROS node with `node_options`.
   See `rclcpp::Node::Node` documentation for further reference on arguments.
   */
  DrakeRos(const std::string& node_name,
           rclcpp::NodeOptions node_options = rclcpp::NodeOptions{});

  ~DrakeRos();

  /** Returns a constant reference to the underlying ROS node. */
  const rclcpp::Node& get_node() const;

  /** Returns a mutable reference to the underlying ROS node. */
  rclcpp::Node* get_mutable_node() const;

  /** Spins the underlying ROS node, dispatching all available work if any.

   In this context, work refers to subscription callbacks, timer callbacks,
   service request and reply callbacks, etc., that are registered with the
   underlying `rclcpp::Node` instance. Availability implies these are ready
   to be serviced by the underlying `rclcpp::Executor` instance.

   This method's behavior has been modeled after that of the
   `drake::lcm::DrakeLcm::HandleSubscriptions()` method (to a partial extent).

   @param[in] timeout_millis Timeout, in milliseconds, when fetching work.
     Negative timeout values are not allowed. If timeout is 0, the call will
     not wait for any new work. If timeout is larger than 0, the call will
     continue fetching work up to the given timeout or until no work is
     available.
   @throws std::runtime_error if timeout is negative.
   */
  void Spin(int timeout_millis = 0);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**Initialize Drake ROS's global context.
 This function decorates a `rclcpp::init` invocation.
*/
void init(int argc = 0, const char** argv = nullptr);

/**Shutdown Drake ROS's global context.
 This function decorates a `rclcpp::shutdown` invocation.
*/
bool shutdown();

}  // namespace drake_ros_core
