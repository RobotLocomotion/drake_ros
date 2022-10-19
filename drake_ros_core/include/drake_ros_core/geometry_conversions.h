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

/**
@file

Conversions for ROS's geometry_msgs types for Drake and Eigen types that Drake
uses.

### Type Suffixes

Note that quantities like `Eigen::Vector3d`, `Eigen::Isometry3d`,
`drake::math::RigidTransformd`, etc. all have the `d` suffix to denote
templating on `double`.

For simplicity, we strip these suffixes from the names in the conversion
functions.

### Spatial Vectors

All 6d spatial vectors are ordered by [rotational; translational], per
ttps://drake.mit.edu/doxygen_cxx/group__multibody__spatial__vectors.html

### Isometry3 Elements

All Isometry3<> conversion methods actually use RigidTransform as an
intermediate. This means that we are restricted to SE(3), or more specifically,
rotations are constrained to be in SO(3), not just O(3), so it is not to
express mirroring in Isometry3<> results (even though the type is capable of
expressing it).
*/

#pragma once

#include <Eigen/Geometry>
#include <drake/common/eigen_types.h>
#include <drake/math/rigid_transform.h>
#include <drake/multibody/math/spatial_algebra.h>
#include <geometry_msgs/msg/accel.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/quaternion.hpp>
#include <geometry_msgs/msg/transform.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/wrench.hpp>

namespace drake_ros_core {

// Vector / Translation.

Eigen::Vector3d RosPointToVector3(const geometry_msgs::msg::Point& point);

geometry_msgs::msg::Point Vector3ToRosPoint(const Eigen::Vector3d& point);

Eigen::Vector3d RosVector3ToVector3(const geometry_msgs::msg::Vector3& point);

geometry_msgs::msg::Vector3 Vector3ToRosVector3(const Eigen::Vector3d& point);

// Orientation.

Eigen::Quaternion<double> RosQuaternionToQuaternion(
    const geometry_msgs::msg::Quaternion& quat);

geometry_msgs::msg::Quaternion QuaternionToRosQuaternion(
    const Eigen::Quaternion<double>& quat);

drake::math::RotationMatrixd RosQuaternionToRotationMatrix(
    const geometry_msgs::msg::Quaternion& quat);

geometry_msgs::msg::Quaternion RotationMatrixToRosQuaternion(
    const drake::math::RotationMatrixd& rotation);

// Pose.

drake::math::RigidTransformd RosPoseToRigidTransform(
    const geometry_msgs::msg::Pose& pose);

geometry_msgs::msg::Pose RigidTransformToRosPose(
    const drake::math::RigidTransformd& transform);

drake::math::RigidTransformd RosTransformToRigidTransform(
    const geometry_msgs::msg::Transform& transform);

geometry_msgs::msg::Transform RigidTransformToRosTransform(
    const drake::math::RigidTransformd& transform);

Eigen::Isometry3d RosPoseToIsometry3(const geometry_msgs::msg::Pose& pose);

geometry_msgs::msg::Pose Isometry3ToRosPose(const Eigen::Isometry3d& isometry);

Eigen::Isometry3d RosTransformToIsometry3(
    const geometry_msgs::msg::Transform& transform);

geometry_msgs::msg::Transform Isometry3ToRosTransform(
    const Eigen::Isometry3d& isometry);

// Spatial Velocity.

drake::Vector6d RosTwistToVector6(const geometry_msgs::msg::Twist& twist);

geometry_msgs::msg::Twist Vector6ToRosTwist(const drake::Vector6d& vector);

drake::multibody::SpatialVelocity<double> RosTwistToSpatialVelocity(
    const geometry_msgs::msg::Twist& twist);

geometry_msgs::msg::Twist SpatialVelocityToRosTwist(
    const drake::multibody::SpatialVelocity<double>& velocity);

// Spatial Acceleration.

drake::Vector6d RosAccelToVector6(const geometry_msgs::msg::Accel& accel);

geometry_msgs::msg::Accel Vector6ToRosAccel(const drake::Vector6d& vector);

drake::multibody::SpatialAcceleration<double> RosAccelToSpatialAcceleration(
    const geometry_msgs::msg::Accel& accel);

geometry_msgs::msg::Accel SpatialAccelerationToRosAccel(
    const drake::multibody::SpatialAcceleration<double>& accel);

// Spatial Force.

drake::Vector6d RosWrenchToVector6(const geometry_msgs::msg::Wrench& wrench);

geometry_msgs::msg::Wrench Vector6ToRosWrench(const drake::Vector6d& vector);

drake::multibody::SpatialForce<double> RosWrenchToSpatialForce(
    const geometry_msgs::msg::Wrench& wrench);

geometry_msgs::msg::Wrench SpatialForceToRosWrench(
    const drake::multibody::SpatialForce<double>& force);

}  // namespace drake_ros_core
