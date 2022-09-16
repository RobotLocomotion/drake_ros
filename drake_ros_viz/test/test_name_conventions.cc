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

TEST(NameConventions, CalcMarkerNamespace) {
  EXPECT_EQ("prefix_geometry_owning_source_name",
            drake_ros_viz::internal::CalcMarkerNamespace(
                "prefix_", "geometry_owning_source_name"));

  EXPECT_EQ("geometry_owning_source_name",
            drake_ros_viz::internal::CalcMarkerNamespace(
                "", "geometry_owning_source_name"));

  EXPECT_EQ("prefix/model/geometry_owning_source_name",
            drake_ros_viz::internal::CalcMarkerNamespace(
                "prefix/", "model::geometry_owning_source_name"));
}

TEST(NameConventions, CalcHierarchicalMarkerNamespaceWithBody) {
  // All names are present
  EXPECT_EQ("prefix/model_name/body_name/geometry_name",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "prefix/", "model_name", "body_name", "geometry_name"));

  // Scoped model name and body name
  EXPECT_EQ("prefix/model_scope/model_name/body_scope/body_name/geometry_name",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "prefix/", "model_scope::model_name", "body_scope::body_name",
                "geometry_name"));

  // Model name with empty body name
  EXPECT_EQ("model_name/unnamed_body/geometry_name",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "", "model_name", "", "geometry_name"));

  // Scoped model name with empty body name and empty geometry name
  EXPECT_EQ("model_scope/model_name/unnamed_body/unnamed_geometry",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "", "model_scope::model_name", "", ""));
}

TEST(NameConventions, CalcHierarchicalMarkerNamespaceWithoutBody) {
  // Geometry source and name is not empty
  EXPECT_EQ("prefix/geometry_source/geometry_name",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "prefix/", "geometry_source", "geometry_name"));

  // Geometry name is empty
  EXPECT_EQ("prefix/geometry_source/unnamed_geometry",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "prefix/", "geometry_source", ""));

  // Geometry name with / delimiters
  EXPECT_EQ("geometry_source/scope_one/scope_two/geometry_name",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "", "geometry_source", "scope_one/scope_two/geometry_name"));

  // Geometry name with :: delimiters
  EXPECT_EQ("geometry_source/scope_one/scope_two/geometry_name",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "", "geometry_source", "scope_one::scope_two::geometry_name"));

  // Geometry name with mixed delimiters
  EXPECT_EQ("geometry_source/scope_one/scope_two/geometry_name",
            drake_ros_viz::internal::CalcHierarchicalMarkerNamespace(
                "", "geometry_source", "scope_one/scope_two::geometry_name"));
}
