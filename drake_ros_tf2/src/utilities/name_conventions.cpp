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

#include "drake_ros_tf2/utilities/name_conventions.hpp"

#include <sstream>
#include <string>
#include <unordered_set>

namespace drake_ros_tf2 {
namespace utilities {

namespace {

std::string ReplaceAllOccurrences(std::string string, const std::string& target,
                                  const std::string& replacement) {
  std::string::size_type n = 0;
  while ((n = string.find(target, n)) != std::string::npos) {
    string.replace(n, target.size(), replacement);
    n += replacement.size();
  }
  return string;
}

}  // namespace

std::string GetTfFrameName(
    const drake::geometry::SceneGraphInspector<double>& inspector,
    const std::unordered_set<const drake::multibody::MultibodyPlant<double>*>&
        plants,
    const drake::geometry::FrameId& frame_id) {
  std::stringstream ss;
  for (auto* plant : plants) {
    const drake::multibody::Body<double>* body =
        plant->GetBodyFromFrameId(frame_id);
    if (!body) {
      continue;
    }
    const std::string& model_instance_name =
        plant->GetModelInstanceName(body->model_instance());
    ss << model_instance_name << "/";
    const std::string& body_name = body->name();
    if (body_name.empty()) {
      ss << "unnamed_body_" << body->index();
    } else {
      ss << ReplaceAllOccurrences(body_name, "::", "/");
    }
    return ss.str();
  }
  const std::string& frame_name = inspector.GetName(frame_id);
  if (frame_name.empty()) {
    ss << "unnamed_frame_" << frame_id;
  } else {
    ss << ReplaceAllOccurrences(frame_name, "::", "/");
  }
  return ss.str();
}

std::string GetTfFrameName(
    const drake::geometry::SceneGraphInspector<double>& inspector,
    const std::unordered_set<const drake::multibody::MultibodyPlant<double>*>&
        plants,
    const drake::geometry::GeometryId& geometry_id) {
  return GetTfFrameName(inspector, plants, inspector.GetFrameId(geometry_id));
}

}  // namespace utilities
}  // namespace drake_ros_tf2
