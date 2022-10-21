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
#include <optional>  // NOLINT(build/include_order)
#include <string>

#include <drake/geometry/geometry_roles.h>
#include <drake/geometry/rgba.h>
#include <drake/multibody/plant/multibody_plant.h>
#include <drake/systems/framework/leaf_system.h>
#include <drake_ros_viz/name_conventions.h>
#include <visualization_msgs/msg/marker_array.hpp>

namespace drake_ros_viz {

/// Set of parameters that configure a SceneMarkersSystem.
struct SceneMarkersParams {
  /// Configure SceneMarkersSystem to depict illustration geometries.
  static SceneMarkersParams Illustration() { return {}; }

  /// Configure SceneMarkersSystem to depict proximity geometries.
  static SceneMarkersParams Proximity() {
    SceneMarkersParams params;
    params.role = drake::geometry::Role::kProximity;
    params.default_color.set(0.5, 0.5, 0.5, 1.0);
    return params;
  }

  /// Function for generating marker namespaces.
  MarkerNamespaceFunction marker_namespace_function =
      GetFlatMarkerNamespaceFunction();

  /// Role of the geometries to depict.
  drake::geometry::Role role{drake::geometry::Role::kIllustration};

  /// Default marker color if no ("phong", "diffuse") property is found.
  drake::geometry::Rgba default_color{0.9, 0.9, 0.9, 1.0};
};

/// System for SceneGraph depiction as a ROS marker array.
///
/// This system outputs a `visualization_msgs/msg/MarkerArray` populated with
/// all geometries found in a SceneGraph, using Context time to timestamp
/// each `visualization_msgs/msg/Marker` message.
///
/// It has one input port:
/// - *graph_query* (abstract): expects a QueryObject from the SceneGraph.
///
/// It has one output port:
/// - *scene_markers* (abstract): all scene geometries, as a
///   visualization_msg::msg::MarkerArray message.
///
/// This system provides the same base functionality in terms of SceneGraph
/// geometries lookup and message conversion for ROS-based applications as
/// the DrakeVisualizer system does for LCM-based applications.
/// Thus, discussions in DrakeVisualizer's documentation about geometry
/// versioning, geometry roles, and so on are equally applicable here.
class SceneMarkersSystem : public drake::systems::LeafSystem<double> {
 public:
  explicit SceneMarkersSystem(SceneMarkersParams params = {});
  virtual ~SceneMarkersSystem();

  /// Register a MultibodyPlant present in the scene
  /**
   * This provides the system with additional information to generate
   * semantically meaningful frame string IDs and marker namespaces.
   */
  void RegisterMultibodyPlant(
      const drake::multibody::MultibodyPlant<double>* plant);

  const SceneMarkersParams& params() const;

  const drake::systems::InputPort<double>& get_graph_query_input_port() const;

  const drake::systems::OutputPort<double>& get_markers_output_port() const;

 private:
  // Outputs visualization_msgs::msg::MarkerArray message,
  // timestamping the most up-to-date version.
  void PopulateSceneMarkersMessage(
      const drake::systems::Context<double>& context,
      visualization_msgs::msg::MarkerArray* output_value) const;

  // Returns cached visualization_msgs::msg::MarkerArray message,
  // which is invalidated (and thus recomputed) upon a SceneGraph
  // geometry version change.
  const visualization_msgs::msg::MarkerArray& EvalSceneMarkers(
      const drake::systems::Context<double>& context,
      bool* cached = nullptr) const;

  // Inspects the SceneGraph and carries out the conversion
  // to visualization_msgs::msg::MarkerArray message unconditionally.
  void CalcSceneMarkers(
      const drake::systems::Context<double>& context,
      visualization_msgs::msg::MarkerArray* output_value) const;

  // PIMPL forward declaration
  class SceneMarkersSystemPrivate;

  std::unique_ptr<SceneMarkersSystemPrivate> impl_;
};

}  // namespace drake_ros_viz
