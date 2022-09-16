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

#include <drake/math/rigid_transform.h>
#include <geometry_msgs/msg/transform.hpp>

namespace drake_ros_tf2 {

/** Convert `X_AB` into a Transform ROS message. */
geometry_msgs::msg::Transform ToTransformMsg(
    const drake::math::RigidTransform<double> X_AB);

}  // namespace drake_ros_tf2
