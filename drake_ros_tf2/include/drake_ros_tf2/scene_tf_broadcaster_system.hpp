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
#ifndef DRAKE_ROS_TF2__SCENE_TF_BROADCASTER_SYSTEM_HPP_
#define DRAKE_ROS_TF2__SCENE_TF_BROADCASTER_SYSTEM_HPP_

#include <memory>
#include <unordered_set>

#include <drake/multibody/plant/multibody_plant.h>
#include <drake/systems/framework/leaf_system.h>

#include "drake_ros_core/drake_ros_interface.hpp"

namespace drake_ros_tf2 {

/// Set of parameters that configure a SceneTfBroadcasterParams.
struct SceneTfBroadcasterParams {
  /// Publish triggers for tf broadcasting.
  std::unordered_set<drake::systems::TriggerType> publish_triggers{
      drake::systems::TriggerType::kForced,
      drake::systems::TriggerType::kPerStep};

  /// Period for periodic tf broadcasting.
  double publish_period{0.0};
};

/// System for tf2 transform broadcasting.
///
/// This system is a subdiagram aggregating a SceneTfSystem and a
/// RosPublisherSystem to broadcast SceneGraph frame transforms.
/// Messages are published to the `/tf` ROS topic.
///
/// It exports one input port:
/// - *graph_query* (abstract): expects a QueryObject from the SceneGraph.
class SceneTfBroadcasterSystem : public drake::systems::Diagram<double> {
 public:
  explicit SceneTfBroadcasterSystem(
      std::shared_ptr<drake_ros_core::DrakeRosInterface> ros_interface,
      SceneTfBroadcasterParams params = {});
  ~SceneTfBroadcasterSystem() override;

  // Forwarded to SceneTfSystem::RegisterMultibodyPlant
  void RegisterMultibodyPlant(
      const drake::multibody::MultibodyPlant<double>* plant);

  const drake::systems::InputPort<double>& get_graph_query_port() const;

 private:
  class SceneTfBroadcasterSystemPrivate;

  std::unique_ptr<SceneTfBroadcasterSystemPrivate> impl_;
};
}  // namespace drake_ros_tf2
#endif  // DRAKE_ROS_TF2__SCENE_TF_BROADCASTER_SYSTEM_HPP_
