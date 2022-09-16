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

#include <sstream>
#include <string>

namespace drake_ros_tf2 {
namespace internal {

std::string ReplaceAllOccurrences(std::string string, const std::string& target,
                                  const std::string& replacement) {
  std::string::size_type n = 0;
  while ((n = string.find(target, n)) != std::string::npos) {
    string.replace(n, target.size(), replacement);
    n += replacement.size();
  }
  return string;
}

/* Formulate tf frame name given the model instance name, body name, body
  index and frame ID value.
  @param[in] model_instance_name name of a given model instance.
  @param[in] body_name name of a given body.
  @param[in] body_index index of a given body.
  @param[in] frame_id_value value of the given frame ID.
  @returns formulated tf frame name.
 */
template <typename ElementIndexType>
std::string CalcTfFrameName(const std::string& model_instance_name,
                            const std::string& body_name,
                            ElementIndexType body_index,
                            int64_t frame_id_value) {
  std::stringstream ss;
  ss << ReplaceAllOccurrences(model_instance_name, "::", "/") << "/";

  if (body_name.empty()) {
    ss << "unnamed_body_" << body_index << "/";
  } else {
    ss << ReplaceAllOccurrences(body_name, "::", "/") << "/";
  }

  ss << frame_id_value;
  return ss.str();
}

/* Formulate tf frame name given the frame name and frame ID value.
  @param[in] frame_name name of the given frame.
  @param[in] frame_id_value value of the given frame ID.
  @returns formulated tf frame name.
 */
std::string CalcTfFrameName(const std::string& frame_name,
                            int64_t frame_id_value) {
  std::stringstream ss;
  if (frame_name.empty() || frame_name == "/" || frame_name == "::") {
    ss << "unnamed_frame_" << frame_id_value;
  } else {
    ss << ReplaceAllOccurrences(frame_name, "::", "/");
  }

  return ss.str();
}

}  // namespace internal
}  // namespace drake_ros_tf2
