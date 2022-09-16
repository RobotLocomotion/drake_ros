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

#include "drake_ros_tf2/utilities/name_conventions.h"

#include <sstream>
#include <string>
#include <unordered_set>

#include "utilities/internal_name_conventions.h"

namespace drake_ros_tf2 {

std::string GetTfFrameName(
    const drake::geometry::SceneGraphInspector<double>& inspector,
    const std::unordered_set<const drake::multibody::MultibodyPlant<double>*>&
        plants,
    const drake::geometry::FrameId& frame_id) {
  // Special case: world frame is always world
  if (frame_id == inspector.world_frame_id()) {
    return "world";
  }

  std::stringstream ss;
  for (auto* plant : plants) {
    const drake::multibody::Body<double>* body =
        plant->GetBodyFromFrameId(frame_id);
    if (!body) {
      continue;
    }

    return internal::CalcTfFrameName(
        plant->GetModelInstanceName(body->model_instance()), body->name(),
        body->index(), frame_id.get_value());
  }

  return internal::CalcTfFrameName(inspector.GetName(frame_id),
                                   frame_id.get_value());
}

std::string GetTfFrameName(
    const drake::geometry::SceneGraphInspector<double>& inspector,
    const std::unordered_set<const drake::multibody::MultibodyPlant<double>*>&
        plants,
    const drake::geometry::GeometryId& geometry_id) {
  return GetTfFrameName(inspector, plants, inspector.GetFrameId(geometry_id));
}

}  // namespace drake_ros_tf2
