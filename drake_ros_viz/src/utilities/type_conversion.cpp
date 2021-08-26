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

#include "drake_ros_viz/utilities/type_conversion.hpp"

#include <drake/common/eigen_types.h>
#include <drake/math/rigid_transform.h>
#include <geometry_msgs/msg/pose.hpp>

namespace drake_ros_viz {
namespace utilities {

geometry_msgs::msg::Pose ToPoseMsg(
    const drake::math::RigidTransform<double> X_AB) {
  geometry_msgs::msg::Pose msg;

  const drake::Vector3<double>& p_AB = X_AB.translation();
  msg.position.x = p_AB.x();
  msg.position.y = p_AB.y();
  msg.position.z = p_AB.z();
  const drake::Quaternion<double> R_AB = X_AB.rotation().ToQuaternion();
  msg.orientation.x = R_AB.x();
  msg.orientation.y = R_AB.y();
  msg.orientation.z = R_AB.z();
  msg.orientation.w = R_AB.w();

  return msg;
}

}  // namespace utilities
}  // namespace drake_ros_viz
