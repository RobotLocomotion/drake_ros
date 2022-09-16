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

#include "drake_ros_viz/utilities/name_conventions.h"

#include <sstream>
#include <string>
#include <unordered_set>

#include "utilities/internal_name_conventions.h"

namespace drake_ros_viz {

MarkerNamespaceFunction GetFlatMarkerNamespaceFunction(
    const std::optional<std::string>& marker_namespace_prefix) {
  return [prefix = marker_namespace_prefix.value_or("")](
             const drake::geometry::SceneGraphInspector<double>& inspector,
             const std::unordered_set<
                 const drake::multibody::MultibodyPlant<double>*>&,
             const drake::geometry::GeometryId geometry_id) {
    return internal::CalcMarkerNamespace(
        prefix, inspector.GetOwningSourceName(geometry_id));
  };
}

// TODO(aaronchongth): GetName provides more components than just the base
// geometry name, as shown in
// https://github.com/EricCousineau-TRI/repro/commit/c44615ee. This
// will need to be refined further, perhaps just extracting the base name
// without any scopes.
MarkerNamespaceFunction GetHierarchicalMarkerNamspaceFunction(
    const std::optional<std::string>& marker_namespace_prefix) {
  return [prefix = marker_namespace_prefix.value_or("")](
             const drake::geometry::SceneGraphInspector<double>& inspector,
             const std::unordered_set<
                 const drake::multibody::MultibodyPlant<double>*>& plants,
             const drake::geometry::GeometryId geometry_id) {
    for (auto* plant : plants) {
      const drake::multibody::Body<double>* body =
          plant->GetBodyFromFrameId(inspector.GetFrameId(geometry_id));
      if (!body) {
        continue;
      }

      return internal::CalcHierarchicalMarkerNamespace(
          prefix, plant->GetModelInstanceName(body->model_instance()),
          body->name(), inspector.GetName(geometry_id));
    }

    return internal::CalcHierarchicalMarkerNamespace(
        prefix, inspector.GetOwningSourceName(geometry_id),
        inspector.GetName(geometry_id));
  };
}

}  // namespace drake_ros_viz
