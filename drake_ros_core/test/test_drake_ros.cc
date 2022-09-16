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

#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include "drake_ros_core/drake_ros.h"

using drake_ros_core::DrakeRos;

TEST(DrakeRos, default_construct) {
  drake_ros_core::init();
  EXPECT_NO_THROW(std::make_unique<DrakeRos>("default_node"));
  EXPECT_TRUE(drake_ros_core::shutdown());
}

TEST(DrakeRos, local_context) {
  auto context = std::make_shared<rclcpp::Context>();
  rclcpp::NodeOptions node_options;
  node_options.context(context);
  context->init(0, nullptr);
  auto drake_ros = std::make_unique<DrakeRos>("local_ctx_node", node_options);
  (void)drake_ros;
  // Should not have initialized global context
  EXPECT_FALSE(rclcpp::contexts::get_global_default_context()->is_valid());
  context->shutdown("done");
}

// Only available in Bazel.
#ifndef _TEST_DISABLE_RMW_ISOLATION
#include "rmw_isolation/rmw_isolation.h"

int main(int argc, char* argv[]) {
  const char* TEST_TMPDIR = std::getenv("TEST_TMPDIR");
  if (TEST_TMPDIR != nullptr) {
    std::string ros_home = std::string(TEST_TMPDIR) + "/.ros";
    setenv("ROS_HOME", ros_home.c_str(), 1);
    ros2::isolate_rmw_by_path(argv[0], TEST_TMPDIR);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
