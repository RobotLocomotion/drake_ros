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
#ifndef DRAKE_ROS_VIZ__RVIZ_VISUALIZER_HPP_
#define DRAKE_ROS_VIZ__RVIZ_VISUALIZER_HPP_

#include <memory>
#include <unordered_set>

#include <drake/multibody/plant/multibody_plant.h>
#include <drake/systems/framework/diagram.h>
#include <drake_ros_core/drake_ros_interface.hpp>

namespace drake_ros_viz {

/// Set of parameters that configure an RvizVisualizer.
struct RvizVisualizerParams {
  /// Publish triggers for scene markers and tf broadcasting.
  std::unordered_set<drake::systems::TriggerType> publish_triggers{
      drake::systems::TriggerType::kForced,
      drake::systems::TriggerType::kPeriodic};

  /// Period for periodic scene markers and tf broadcasting.
  double publish_period{0.05};

  /// Whether to perform tf broadcasting or not.
  bool publish_tf{true};
};

/// System for SceneGraph visualization in RViz.
///
/// This system is a subdiagram aggregating a SceneMarkersSystem, a
/// RosPublisherSystem, and optionally a TfBroadcasterSystem to enable
/// SceneGraph visualization in RViz. All scene geometries are published
/// as a visualization_msgs/msg/MarkerArray message to a `/scene_markers`
/// ROS topic. If `publish_tf` is `true`, all SceneGraph frames are
/// broadcasted as tf2 transforms.
///
/// It exports one input port:
/// - *graph_query* (abstract): expects a QueryObject from the SceneGraph.
class RvizVisualizer : public drake::systems::Diagram<double> {
 public:
  explicit RvizVisualizer(
      std::shared_ptr<drake_ros_core::DrakeRosInterface> ros_interface,
      RvizVisualizerParams params = {});

  ~RvizVisualizer() override;

  // Forwarded to SceneTfSystem::RegisterMultibodyPlant
  // and SceneTfBroadcasterSystem::RegisterMultibodyPlant
  void RegisterMultibodyPlant(
      const drake::multibody::MultibodyPlant<double>* plant);

  const drake::systems::InputPort<double>& get_graph_query_port() const;

 private:
  class RvizVisualizerPrivate;

  std::unique_ptr<RvizVisualizerPrivate> impl_;
};

}  // namespace drake_ros_viz

#endif  // DRAKE_ROS_VIZ__RVIZ_VISUALIZER_HPP_
