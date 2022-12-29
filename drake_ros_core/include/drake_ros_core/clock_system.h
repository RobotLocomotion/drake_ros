// Copyright 2022 Open Source Robotics Foundation, Inc.
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
#include <tuple>
#include <unordered_set>

#include <drake/systems/framework/diagram_builder.h>
#include <drake/systems/framework/leaf_system.h>
#include <drake_ros_core/ros_publisher_system.h>
#include <rclcpp/qos.hpp>
#include <rosgraph_msgs/msg/clock.hpp>

namespace drake_ros_core {
/** A system that convert's drake's time to a rosgraph_msgs/msg/Clock message.
 */
class ClockSystem : public drake::systems::LeafSystem<double> {
 public:
  /** A constructor for the clock system.
   */
  ClockSystem();

  ~ClockSystem() override;

  /** Add a ClockSystem and RosPublisherSystem to a diagram builder.
   *
   * This adds both a ClockSystem and a RosPublisherSystem that publishes
   * time to a `/clock` topic. All nodes should have their `use_sim_time`
   * parameter set to `True` so they use the published topic as their source
   * of time.
   */
  static std::tuple<ClockSystem*, RosPublisherSystem*> AddToBuilder(
      drake::systems::DiagramBuilder<double>* builder, DrakeRos* ros,
      const std::string& topic_name = "/clock",
      const rclcpp::QoS& qos = rclcpp::ClockQoS(),
      const std::unordered_set<drake::systems::TriggerType>& publish_triggers =
          RosPublisherSystem::kDefaultTriggerTypes,
      double publish_period = 0.0);

 protected:
  void CalcClock(const drake::systems::Context<double>& context,
                 rosgraph_msgs::msg::Clock* output_value) const;
};
}  // namespace drake_ros_core
