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

#include <optional>
#include <string>
#include <unordered_set>

#include <drake/geometry/scene_graph_inspector.h>
#include <drake/multibody/plant/multibody_plant.h>

namespace drake_ros_viz {

/** A functor that returns the marker namespace given information of a geometry.
  @param[in] inspector inspector for a given SceneGraph's data.
  @param[in] plants a set of MultibodyPlant instances from which to derive
    semantically meaningful marker namespaces, if possible.
  @param[in] geometry_id target geometry ID.
 */
using MarkerNamespaceFunction = std::function<std::string(
    const drake::geometry::SceneGraphInspector<double>&,
    const std::unordered_set<const drake::multibody::MultibodyPlant<double>*>&,
    const drake::geometry::GeometryId)>;

/** Returns a functor that generates a flat, non-hierarchical marker namespace,
  comprised of the owning source name of the geometry, prefixed optionally by
  marker_namespace_prefix.
  @param[in] marker_namespace_prefix Optional prefix for the marker.
  @returns the functor for generating the marker namespace.
 */
MarkerNamespaceFunction GetFlatMarkerNamespaceFunction(
    const std::optional<std::string>& marker_namespace_prefix = std::nullopt);

/** Returns a functor that generates a hierarchical namespace (if possible) in
  the form of 'model_instance_name/body_name/geometry_name'. This is
  prefixed optionally by marker_namespace_prefix.
  @param[in] marker_namespace_prefix Optional prefix for the marker.
  @returns the functor for generating the marker namespace.
 */
MarkerNamespaceFunction GetHierarchicalMarkerNamespaceFunction(
    const std::optional<std::string>& marker_namespace_prefix = std::nullopt);

}  // namespace drake_ros_viz
