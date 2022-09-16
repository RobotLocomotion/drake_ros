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
#include <unordered_set>
#include <utility>

#include <drake/systems/framework/leaf_system.h>
#include <rclcpp/serialized_message.hpp>
#include <rmw/rmw.h>

#include "drake_ros_core/drake_ros.h"
#include "drake_ros_core/serializer.h"
#include "drake_ros_core/serializer_interface.h"

namespace drake_ros_core {
/** A system that can publish ROS messages.
 It accepts ROS messages on its sole input port and publishes them
 to a ROS topic.
 */
class RosPublisherSystem : public drake::systems::LeafSystem<double> {
 public:
  static constexpr std::initializer_list<drake::systems::TriggerType>
      kDefaultTriggerTypes{drake::systems::TriggerType::kPerStep,
                           drake::systems::TriggerType::kForced};

  /** Instantiates a publisher system for a given ROS message type.
   See `RosPublisherSystem::RosPublisherSystem` documentation for
   further reference on function arguments.

   @tparam MessageT C++ ROS message type.
   */
  template <typename MessageT>
  static std::unique_ptr<RosPublisherSystem> Make(
      const std::string& topic_name, const rclcpp::QoS& qos, DrakeRos* ros,
      const std::unordered_set<drake::systems::TriggerType>& publish_triggers =
          kDefaultTriggerTypes,
      double publish_period = 0.0) {
    // Assume C++ typesupport since this is a C++ template function
    return std::make_unique<RosPublisherSystem>(
        std::make_unique<Serializer<MessageT>>(), topic_name, qos, ros,
        publish_triggers, publish_period);
  }

  /** A constructor for the ROS publisher system.
   It takes a `serializer` to deal with outgoing messages.

   @param[in] serializer a (de)serialization interface for the
     expected ROS message type.
   @param[in] topic_name Name of the ROS topic to publish to.
   @param[in] qos QoS profile for the underlying ROS pubslisher.
   @param[in] ros interface to a live ROS node to publish from.
   @param[in] publish_triggers optional set of triggers that determine
     when messages will be published. By default it will publish on
     every step and when forced (via Publish).
   @param[in] publish_period optional publishing period, in seconds.
     Only applicable when periodic publishing is enabled.
   */
  RosPublisherSystem(std::unique_ptr<SerializerInterface> serializer,
                     const std::string& topic_name, const rclcpp::QoS& qos,
                     DrakeRos* ros,
                     const std::unordered_set<drake::systems::TriggerType>&
                         publish_triggers = kDefaultTriggerTypes,
                     double publish_period = 0.0);

  ~RosPublisherSystem() override;

  /** Publishes a C++ ROS `message`. */
  template <typename MessageT>
  void Publish(const MessageT& message) {
    static const Serializer<MessageT> serializer;
    publish(serializer.serialize(message));
  }

  /** Publishes a serialized ROS message. */
  void Publish(const rclcpp::SerializedMessage& serialized_message);

 protected:
  drake::systems::EventStatus PublishInput(
      const drake::systems::Context<double>& context) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace drake_ros_core
