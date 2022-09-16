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

#include <cmath>

#include <gtest/gtest.h>

#include "drake_ros_viz/utilities/type_conversion.h"

TEST(TypeConversion, ToPoseMsg) {
  const drake::Vector3<double> position{1., 2., 3.};
  const drake::Quaternion<double> orientation{0., 0., sin(M_PI / 4.),
                                              cos(M_PI / 4.)};
  const drake::math::RigidTransform<double> pose{orientation, position};
  const geometry_msgs::msg::Pose message = drake_ros_viz::ToPoseMsg(pose);
  EXPECT_DOUBLE_EQ(message.position.x, position.x());
  EXPECT_DOUBLE_EQ(message.position.y, position.y());
  EXPECT_DOUBLE_EQ(message.position.z, position.z());
  EXPECT_DOUBLE_EQ(message.orientation.x, orientation.x());
  EXPECT_DOUBLE_EQ(message.orientation.y, orientation.y());
  EXPECT_DOUBLE_EQ(message.orientation.z, orientation.z());
  EXPECT_DOUBLE_EQ(message.orientation.w, orientation.w());
}
