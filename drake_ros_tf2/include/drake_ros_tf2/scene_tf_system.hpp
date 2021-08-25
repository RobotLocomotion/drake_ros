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
#ifndef DRAKE_ROS_TF2__SCENE_TF_SYSTEM_HPP_
#define DRAKE_ROS_TF2__SCENE_TF_SYSTEM_HPP_

#include <memory>

#include <drake/multibody/plant/multibody_plant.h>
#include <drake/systems/framework/context.h>
#include <drake/systems/framework/leaf_system.h>
#include <tf2_msgs/msg/tf_message.hpp>

namespace drake_ros_tf2 {
/// System for SceneGraph frame transforms aggregation as a ROS tf2 message.
///
/// This system outputs a `tf2_msgs/msg/TFMessage` populated with the
/// rigid transforms from the world frame to each other frame in the scene,
/// using Context time to timestamp each `geometry_msgs/msg/TransformStamped`
/// message.
///
/// It has one input port:
/// - *graph_query* (abstract): expects a QueryObject from the SceneGraph.
///
/// It has one output port:
/// - *scene_tf* (abstract): rigid transforms w.r.t. the world frame for all
///   frames in the scene, as a tf2_msgs::msg::TFMessage message.
class SceneTfSystem : public drake::systems::LeafSystem<double> {
 public:
  SceneTfSystem();
  virtual ~SceneTfSystem();

  /// Register a MultibodyPlant present in the scene
  /**
   * This provides the system with additional information
   * to generate semantically meaningful frame string IDs.
   */
  void RegisterMultibodyPlant(
      const drake::multibody::MultibodyPlant<double>* plant);

  const drake::systems::InputPort<double>& get_graph_query_port() const;

  const drake::systems::OutputPort<double>& get_scene_tf_output_port() const;

 private:
  void CalcSceneTf(const drake::systems::Context<double>& context,
                   tf2_msgs::msg::TFMessage* output_value) const;

  // PIMPL forward declaration
  class SceneTfSystemPrivate;

  std::unique_ptr<SceneTfSystemPrivate> impl_;
};
}  // namespace drake_ros_tf2
#endif  // DRAKE_ROS_TF2__SCENE_TF_SYSTEM_HPP_
