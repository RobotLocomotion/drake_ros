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

#include <string>

#include "utilities/internal_name_conventions.h"
#include <gtest/gtest.h>

#include "drake_ros_tf2/utilities/name_conventions.h"

using drake::geometry::FrameId;
using drake::multibody::ModelInstanceIndex;
using drake::multibody::MultibodyPlant;
using drake::systems::DiagramBuilder;

TEST(NameConventions, CalcTfFrameNameWithBody) {
  // All names are present
  EXPECT_EQ("model_name/body_name/456",
            drake_ros_tf2::internal::CalcTfFrameName<int>(
                "model_name", "body_name", 123, 456));

  // Scoped model name and body name
  EXPECT_EQ("model_scope/model_name/body_scope/body_name/456",
            drake_ros_tf2::internal::CalcTfFrameName(
                "model_scope::model_name", "body_scope::body_name", 123, 456));

  // Model name with empty body name
  EXPECT_EQ(
      "model_name/unnamed_body_123/456",
      drake_ros_tf2::internal::CalcTfFrameName("model_name", "", 123, 456));

  // Scoped model name with empty body name
  EXPECT_EQ("model_scope/model_name/unnamed_body_123/456",
            drake_ros_tf2::internal::CalcTfFrameName("model_scope::model_name",
                                                     "", 123, 456));
}

TEST(NameConventions, CalcTfFrameNameWithoutBody) {
  // Frame name is not empty
  EXPECT_EQ("frame_name",
            drake_ros_tf2::internal::CalcTfFrameName("frame_name", 123));

  // Frame name is empty
  EXPECT_EQ("unnamed_frame_123",
            drake_ros_tf2::internal::CalcTfFrameName("", 123));

  // Frame name with / delimiters
  EXPECT_EQ("scope_one/scope_two/frame_name",
            drake_ros_tf2::internal::CalcTfFrameName(
                "scope_one/scope_two/frame_name", 123));

  // Frame name with :: delimiters
  EXPECT_EQ("scope_one/scope_two/frame_name",
            drake_ros_tf2::internal::CalcTfFrameName(
                "scope_one::scope_two::frame_name", 123));

  // Frame name with mixed delimiters
  EXPECT_EQ("scope_one/scope_two/frame_name",
            drake_ros_tf2::internal::CalcTfFrameName(
                "scope_one/scope_two::frame_name", 123));

  // Frame name with only :: delimiter
  EXPECT_EQ("unnamed_frame_123",
            drake_ros_tf2::internal::CalcTfFrameName("::", 123));

  // Frame name with only / delimiter
  EXPECT_EQ("unnamed_frame_123",
            drake_ros_tf2::internal::CalcTfFrameName("/", 123));
}

TEST(NameConventions, GetTfFrameNameWorld) {
  DiagramBuilder<double> builder;

  auto [plant, scene_graph] = AddMultibodyPlantSceneGraph(
      &builder, std::make_unique<MultibodyPlant<double>>(0.0));

  std::unordered_set<const MultibodyPlant<double>*> plants = {&plant};

  // World frame is always ModelInstanceIndex 0
  const auto world_model_index = ModelInstanceIndex{0};
  FrameId frame_id = plant.GetBodyFrameIdOrThrow(
      plant.GetBodyIndices(world_model_index).at(0));

  EXPECT_EQ("world", drake_ros_tf2::GetTfFrameName(
                         scene_graph.model_inspector(), plants, frame_id));
}
