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

#include <gtest/gtest.h>

#include "drake_ros_core/geometry_conversions.h"

namespace drake_ros_core {
namespace {

// N.B. For the purpose of testing type conversions, we should use explicit
// typing (avoiding auto).

// Simplified version of drake::CompareMatrices (which is not presently
// installed).
[[nodiscard]] ::testing::AssertionResult CompareMatrices(
    const Eigen::MatrixXd& lhs, const Eigen::MatrixXd& rhs,
    double tolerance = 0.0) {
  const auto diff = lhs.array() - rhs.array();
  const auto exceeds = diff.abs() > tolerance;
  if (exceeds.any()) {
    // Maybe transpose for printing.
    auto maybe_transpose = [](Eigen::MatrixXd v) -> Eigen::MatrixXd {
      if (v.rows() > 1 && v.cols() == 1) {
        return v.transpose();
      } else {
        return v;
      }
    };
    return ::testing::AssertionFailure() << fmt::format(
               "Matrices don't match to tolerance of {}.\n"
               "lhs: {}\n\n"
               "rhs: {}\n\n"
               "lhs - rhs: {}\n\n",
               tolerance, maybe_transpose(lhs), maybe_transpose(rhs),
               maybe_transpose(lhs - rhs));
  }
  return ::testing::AssertionSuccess();
}

// Vector / Translation.

Eigen::Vector3d MakeDummyVector3() {
  // Arbitrary value with distinct elements.
  return Eigen::Vector3d(1.0, 2.0, 3.0);
}

geometry_msgs::msg::Point MakeDummyRosPoint() {
  // Represents same quantity as MakeDummyVector3.
  geometry_msgs::msg::Point message;
  message.x = 1.0;
  message.y = 2.0;
  message.z = 3.0;
  return message;
}

TEST(GeometryConversions, Point) {
  const geometry_msgs::msg::Point message = MakeDummyRosPoint();
  const Eigen::Vector3d value_expected = MakeDummyVector3();
  const Eigen::Vector3d value = RosPointToVector3(message);
  EXPECT_EQ(value, value_expected);
  EXPECT_EQ(message, Vector3ToRosPoint(value));
}

geometry_msgs::msg::Vector3 MakeDummyRosVector3() {
  // Represents same quantity as MakeDummyVector3.
  geometry_msgs::msg::Vector3 message;
  message.x = 1.0;
  message.y = 2.0;
  message.z = 3.0;
  return message;
}

TEST(GeometryConversions, Vector3) {
  const geometry_msgs::msg::Vector3 message = MakeDummyRosVector3();
  const Eigen::Vector3d value_expected = MakeDummyVector3();
  const Eigen::Vector3d value = RosVector3ToVector3(message);
  EXPECT_EQ(value, value_expected);
  EXPECT_EQ(message, Vector3ToRosVector3(value));
}

// Orientation.

Eigen::Quaterniond MakeDummyQuaternion() {
  // This corresponds to RollPitchYaw(np.deg2rad([90, 0, 0]))
  const double w = 1 / sqrt(2), x = 1 / sqrt(2), y = 0.0, z = 0.0;
  return Eigen::Quaterniond(w, x, y, z);
}

geometry_msgs::msg::Quaternion MakeDummyRosQuaternion() {
  // Represents same quantity as MakeDummyQuaternion.
  geometry_msgs::msg::Quaternion message;
  message.w = 1 / sqrt(2);
  message.x = 1 / sqrt(2);
  message.y = 0.0;
  message.z = 0.0;
  return message;
}

[[nodiscard]] ::testing::AssertionResult IsEqual(
    const Eigen::Quaterniond& lhs, const Eigen::Quaterniond& rhs) {
  // N.B. Quaternion::coeffs() returns (x, y, z, w).
  return CompareMatrices(lhs.coeffs(), rhs.coeffs());
}

TEST(GeometryConversions, Quaternion) {
  const geometry_msgs::msg::Quaternion message = MakeDummyRosQuaternion();
  const Eigen::Quaterniond value_expected = MakeDummyQuaternion();
  const Eigen::Quaterniond value = RosQuaternionToQuaternion(message);
  EXPECT_TRUE(IsEqual(value, value_expected));
  EXPECT_EQ(message, QuaternionToRosQuaternion(value));
}

drake::math::RotationMatrixd MakeDummyRotationMatrix() {
  // This corresponds to RollPitchYaw(np.deg2rad([90, 0, 0]))
  Eigen::Matrix3d R;
  R << 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0;
  return drake::math::RotationMatrixd(R);
}

[[nodiscard]] ::testing::AssertionResult IsEqual(
    const drake::math::RotationMatrixd& lhs,
    const drake::math::RotationMatrixd& rhs) {
  return CompareMatrices(lhs.matrix(), rhs.matrix());
}

TEST(GeometryConversions, RotationMatrix) {
  const geometry_msgs::msg::Quaternion message = MakeDummyRosQuaternion();
  const drake::math::RotationMatrixd value_expected = MakeDummyRotationMatrix();
  const drake::math::RotationMatrixd value =
      RosQuaternionToRotationMatrix(message);
  EXPECT_TRUE(IsEqual(value, value_expected));
  EXPECT_EQ(message, RotationMatrixToRosQuaternion(value));
}

// Pose.

drake::math::RigidTransformd MakeDummyRigidTransform() {
  return drake::math::RigidTransformd(MakeDummyQuaternion(),
                                      MakeDummyVector3());
}

geometry_msgs::msg::Pose MakeDummyRosPose() {
  // Represents same quantity as MakeDummyRigidTransform.
  geometry_msgs::msg::Pose message;
  message.position = MakeDummyRosPoint();
  message.orientation = MakeDummyRosQuaternion();
  return message;
}

[[nodiscard]] ::testing::AssertionResult IsEqual(
    const drake::math::RigidTransformd& lhs,
    const drake::math::RigidTransformd& rhs) {
  return CompareMatrices(lhs.GetAsMatrix4(), rhs.GetAsMatrix4());
}

[[nodiscard]] ::testing::AssertionResult IsEqual(const Eigen::Isometry3d& lhs,
                                                 const Eigen::Isometry3d& rhs) {
  return IsEqual(drake::math::RigidTransformd(lhs),
                 drake::math::RigidTransformd(rhs));
}

TEST(GeometryConversions, Pose) {
  const geometry_msgs::msg::Pose message = MakeDummyRosPose();
  const drake::math::RigidTransformd value_expected = MakeDummyRigidTransform();
  const drake::math::RigidTransformd value = RosPoseToRigidTransform(message);
  EXPECT_TRUE(IsEqual(value, value_expected));
  EXPECT_EQ(message, RigidTransformToRosPose(value));

  // Test Isometry3 flavoring.
  EXPECT_EQ(message, Isometry3ToRosPose(value.GetAsIsometry3()));
  EXPECT_TRUE(IsEqual(value.GetAsIsometry3(), RosPoseToIsometry3(message)));
}

geometry_msgs::msg::Transform MakeDummyRosTransform() {
  // Represents same quantity as MakeDummyRigidTransform.
  geometry_msgs::msg::Transform message;
  message.translation = MakeDummyRosVector3();
  message.rotation = MakeDummyRosQuaternion();
  return message;
}

TEST(GeometryConversions, Transform) {
  const geometry_msgs::msg::Transform message = MakeDummyRosTransform();
  const drake::math::RigidTransformd value_expected = MakeDummyRigidTransform();
  const drake::math::RigidTransformd value =
      RosTransformToRigidTransform(message);
  EXPECT_TRUE(IsEqual(value, value_expected));
  EXPECT_EQ(message, RigidTransformToRosTransform(value));

  // Test Isometry3 flavoring.
  EXPECT_TRUE(
      IsEqual(value.GetAsIsometry3(), RosTransformToIsometry3(message)));
  EXPECT_EQ(message, Isometry3ToRosTransform(value.GetAsIsometry3()));
}

// General Spatial Vectors.

// We should distinguish between translational and rotational values.

Eigen::Vector3d MakeDummyVector3ForRotation() { return {0.1, 0.2, 0.3}; }

Eigen::Vector3d MakeDummyVector3ForTranslation() { return {1.0, 2.0, 3.0}; }

geometry_msgs::msg::Vector3 MakeDummyRosVector3ForRotation() {
  // Represents same quantity as MakeDummyVector3ForRotation.
  geometry_msgs::msg::Vector3 value;
  value.x = 0.1;
  value.y = 0.2;
  value.z = 0.3;
  return value;
}

geometry_msgs::msg::Vector3 MakeDummyRosVector3ForTranslation() {
  // Represents same quantity as MakeDummyVector3ForTranslation.
  geometry_msgs::msg::Vector3 value;
  value.x = 1.0;
  value.y = 2.0;
  value.z = 3.0;
  return value;
}

template <template <typename> class SpatialType>
[[nodiscard]] ::testing::AssertionResult IsEqual(
    const drake::multibody::SpatialVector<SpatialType, double>& lhs,
    const drake::multibody::SpatialVector<SpatialType, double>& rhs) {
  return CompareMatrices(lhs.get_coeffs(), rhs.get_coeffs());
}

// Spatial Velocity.

drake::multibody::SpatialVelocity<double> MakeDummySpatialVelocity() {
  return drake::multibody::SpatialVelocity<double>(
      MakeDummyVector3ForRotation(), MakeDummyVector3ForTranslation());
}

geometry_msgs::msg::Twist MakeDummyRosTwist() {
  // Represents same quantity as MakeDummySpatialVelocity.
  geometry_msgs::msg::Twist value;
  value.angular = MakeDummyRosVector3ForRotation();
  value.linear = MakeDummyRosVector3ForTranslation();
  return value;
}

TEST(GeometryConversions, Twist) {
  const geometry_msgs::msg::Twist message = MakeDummyRosTwist();
  const drake::multibody::SpatialVelocity<double> value_expected =
      MakeDummySpatialVelocity();
  const drake::multibody::SpatialVelocity<double> value =
      RosTwistToSpatialVelocity(message);
  EXPECT_TRUE(IsEqual(value, value_expected));
  EXPECT_EQ(message, SpatialVelocityToRosTwist(value));

  // Test Vector6 flavoring.
  EXPECT_EQ(value.get_coeffs(), RosTwistToVector6(message));
  EXPECT_EQ(message, Vector6ToRosTwist(value.get_coeffs()));
}

// Spatial Acceleration.

drake::multibody::SpatialAcceleration<double> MakeDummySpatialAcceleration() {
  return drake::multibody::SpatialAcceleration<double>(
      MakeDummyVector3ForRotation(), MakeDummyVector3ForTranslation());
}

geometry_msgs::msg::Accel MakeDummyRosAccel() {
  // Represents same quantity as MakeDummySpatialAcceleration.
  geometry_msgs::msg::Accel value;
  value.angular = MakeDummyRosVector3ForRotation();
  value.linear = MakeDummyRosVector3ForTranslation();
  return value;
}

TEST(GeometryConversions, Acceleration) {
  const geometry_msgs::msg::Accel message = MakeDummyRosAccel();
  const drake::multibody::SpatialAcceleration<double> value_expected =
      MakeDummySpatialAcceleration();
  const drake::multibody::SpatialAcceleration<double> value =
      RosAccelToSpatialAcceleration(message);
  EXPECT_TRUE(IsEqual(value, value_expected));
  EXPECT_EQ(message, SpatialAccelerationToRosAccel(value));

  // Test Vector6 flavoring.
  EXPECT_EQ(value.get_coeffs(), RosAccelToVector6(message));
  EXPECT_EQ(message, Vector6ToRosAccel(value.get_coeffs()));
}

// Spatial Force.

drake::multibody::SpatialForce<double> MakeDummySpatialForce() {
  return drake::multibody::SpatialForce<double>(
      MakeDummyVector3ForRotation(), MakeDummyVector3ForTranslation());
}

geometry_msgs::msg::Wrench MakeDummyWrench() {
  // Represents same quantity as MakeDummySpatialForce.
  geometry_msgs::msg::Wrench value;
  value.torque = MakeDummyRosVector3ForRotation();
  value.force = MakeDummyRosVector3ForTranslation();
  return value;
}

TEST(GeometryConversions, Force) {
  const geometry_msgs::msg::Wrench message = MakeDummyWrench();
  const drake::multibody::SpatialForce<double> value_expected =
      MakeDummySpatialForce();
  const drake::multibody::SpatialForce<double> value =
      RosWrenchToSpatialForce(message);
  EXPECT_TRUE(IsEqual(value, value_expected));
  EXPECT_EQ(message, SpatialForceToRosWrench(value));

  // Test Vector6 flavoring.
  EXPECT_EQ(value.get_coeffs(), RosWrenchToVector6(message));
  EXPECT_EQ(message, Vector6ToRosWrench(value.get_coeffs()));
}

}  // namespace
}  // namespace drake_ros_core
