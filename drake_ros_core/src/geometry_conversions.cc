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

#include "drake_ros_core/geometry_conversions.h"

namespace drake_ros_core {

Eigen::Vector3d RosPointToVector3(const geometry_msgs::msg::Point& point) {
  return Eigen::Vector3d(point.x, point.y, point.z);
}

geometry_msgs::msg::Point Vector3ToRosPoint(const Eigen::Vector3d& point) {
  geometry_msgs::msg::Point result;
  result.x = point[0];
  result.y = point[1];
  result.z = point[2];
  return result;
}

Eigen::Vector3d RosVector3ToVector3(const geometry_msgs::msg::Vector3& point) {
  return Eigen::Vector3d(point.x, point.y, point.z);
}

geometry_msgs::msg::Vector3 Vector3ToRosVector3(const Eigen::Vector3d& point) {
  geometry_msgs::msg::Vector3 result;
  result.x = point[0];
  result.y = point[1];
  result.z = point[2];
  return result;
}

Eigen::Quaternion<double> RosQuaternionToQuaternion(
    const geometry_msgs::msg::Quaternion& quat) {
  return Eigen::Quaternion<double>(quat.w, quat.x, quat.y, quat.z);
}

geometry_msgs::msg::Quaternion QuaternionToRosQuaternion(
    const Eigen::Quaternion<double>& quat) {
  geometry_msgs::msg::Quaternion result;
  result.x = quat.x();
  result.y = quat.y();
  result.z = quat.z();
  result.w = quat.w();
  return result;
}

drake::math::RotationMatrixd RosQuaternionToRotationMatrix(
    const geometry_msgs::msg::Quaternion& quat) {
  return drake::math::RotationMatrixd(RosQuaternionToQuaternion(quat));
}

geometry_msgs::msg::Quaternion RotationMatrixToRosQuaternion(
    const drake::math::RotationMatrixd& rotation) {
  return QuaternionToRosQuaternion(rotation.ToQuaternion());
}

drake::math::RigidTransformd RosPoseToRigidTransform(
    const geometry_msgs::msg::Pose& pose) {
  return drake::math::RigidTransformd(
      RosQuaternionToQuaternion(pose.orientation),
      RosPointToVector3(pose.position));
}

geometry_msgs::msg::Pose RigidTransformToRosPose(
    const drake::math::RigidTransformd& transform) {
  geometry_msgs::msg::Pose result;
  result.position = Vector3ToRosPoint(transform.translation());
  result.orientation = RotationMatrixToRosQuaternion(transform.rotation());
  return result;
}

drake::math::RigidTransformd RosTransformToRigidTransform(
    const geometry_msgs::msg::Transform& transform) {
  return drake::math::RigidTransformd(
      RosQuaternionToRotationMatrix(transform.rotation),
      RosVector3ToVector3(transform.translation));
}

geometry_msgs::msg::Transform RigidTransformToRosTransform(
    const drake::math::RigidTransformd& transform) {
  geometry_msgs::msg::Transform result;
  result.translation = Vector3ToRosVector3(transform.translation());
  result.rotation = RotationMatrixToRosQuaternion(transform.rotation());
  return result;
}

Eigen::Isometry3d RosPoseToIsometry3(const geometry_msgs::msg::Pose& pose) {
  return RosPoseToRigidTransform(pose).GetAsIsometry3();
}

geometry_msgs::msg::Pose Isometry3ToRosPose(
    const Eigen::Isometry3d& isometry) {
  return RigidTransformToRosPose(drake::math::RigidTransformd(isometry));
}

Eigen::Isometry3d RosTransformToIsometry3(
    const geometry_msgs::msg::Transform& transform) {
  return RosTransformToRigidTransform(transform).GetAsIsometry3();
}

geometry_msgs::msg::Transform Isometry3ToRosTransform(
    const Eigen::Isometry3d& isometry) {
  return RigidTransformToRosTransform(drake::math::RigidTransformd(isometry));
}

drake::Vector6d RosTwistToVector6(const geometry_msgs::msg::Twist& twist) {
  drake::Vector6d result;
  result <<
      RosVector3ToVector3(twist.angular),
      RosVector3ToVector3(twist.linear);
  return result;
}

geometry_msgs::msg::Twist Vector6ToRosTwist(const drake::Vector6d& vector) {
  geometry_msgs::msg::Twist result;
  result.angular = Vector3ToRosVector3(vector.head<3>());
  result.linear = Vector3ToRosVector3(vector.tail<3>());
  return result;
}

drake::multibody::SpatialVelocity<double> RosTwistToSpatialVelocity(
    const geometry_msgs::msg::Twist& twist) {
  return drake::multibody::SpatialVelocity<double>(
      RosVector3ToVector3(twist.angular), RosVector3ToVector3(twist.linear));
}

geometry_msgs::msg::Twist SpatialVelocityToRosTwist(
    const drake::multibody::SpatialVelocity<double>& velocity) {
  geometry_msgs::msg::Twist result;
  result.linear = Vector3ToRosVector3(velocity.translational());
  result.angular = Vector3ToRosVector3(velocity.rotational());
  return result;
}

drake::Vector6d RosAccelToVector6(
    const geometry_msgs::msg::Accel& accel) {
  drake::Vector6d result;
  result <<
      RosVector3ToVector3(accel.angular),
      RosVector3ToVector3(accel.linear);
  return result;
}

geometry_msgs::msg::Accel Vector6ToRosAccel(
    const drake::Vector6d& vector) {
  geometry_msgs::msg::Accel result;
  result.angular = Vector3ToRosVector3(vector.head<3>());
  result.linear = Vector3ToRosVector3(vector.tail<3>());
  return result;
}

drake::multibody::SpatialAcceleration<double> RosAccelToSpatialAcceleration(
    const geometry_msgs::msg::Accel& accel) {
  return drake::multibody::SpatialAcceleration<double>(
      RosVector3ToVector3(accel.angular),
      RosVector3ToVector3(accel.linear));
}

geometry_msgs::msg::Accel SpatialAccelerationToRosAccel(
    const drake::multibody::SpatialAcceleration<double>& accel) {
  geometry_msgs::msg::Accel result;
  result.angular = Vector3ToRosVector3(accel.rotational());
  result.linear = Vector3ToRosVector3(accel.translational());
  return result;
}

drake::Vector6d RosWrenchToVector6(const geometry_msgs::msg::Wrench& wrench) {
  drake::Vector6d result;
  result <<
      RosVector3ToVector3(wrench.torque),
      RosVector3ToVector3(wrench.force);
  return result;
}

geometry_msgs::msg::Wrench Vector6ToRosWrench(const drake::Vector6d& vector) {
  geometry_msgs::msg::Wrench result;
  result.torque = Vector3ToRosVector3(vector.head<3>());
  result.force = Vector3ToRosVector3(vector.tail<3>());
  return result;
}

drake::multibody::SpatialForce<double> RosWrenchToSpatialForce(
    const geometry_msgs::msg::Wrench& wrench) {
  return drake::multibody::SpatialForce<double>(
      RosVector3ToVector3(wrench.torque),
      RosVector3ToVector3(wrench.force));
}

geometry_msgs::msg::Wrench SpatialForceToRosWrench(
    const drake::multibody::SpatialForce<double>& force) {
  geometry_msgs::msg::Wrench result;
  result.torque = Vector3ToRosVector3(force.rotational());
  result.force = Vector3ToRosVector3(force.translational());
  return result;
}

}  // namespace drake_ros_core
