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
#include <string>
#include <utility>

#include <drake/common/value.h>
#include <drake/geometry/geometry_instance.h>
#include <drake/geometry/scene_graph.h>
#include <drake/math/rigid_transform.h>
#include <drake/systems/framework/context.h>
#include <drake/systems/framework/diagram_builder.h>
#include <drake/systems/primitives/constant_value_source.h>
#include <gtest/gtest.h>
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include "drake_ros_viz/scene_markers_system.h"

using drake_ros_viz::SceneMarkersSystem;

static constexpr char kSourceName[] = "test";
static constexpr double kTolerance{1e-6};
static constexpr unsigned int kNumGeometries = 5;

struct SingleSphereSceneTestDetails {
  static constexpr char kName[] = "sphere";
  static constexpr double kRadius{1.};

  static drake::geometry::FramePoseVector<double> PopulateSceneGraph(
      const drake::geometry::SourceId& source_id,
      drake::geometry::SceneGraph<double>* scene_graph) {
    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      std::stringstream ss;
      ss << kName << i;
      const drake::geometry::GeometryId geometry_id =
          scene_graph->RegisterAnchoredGeometry(
              source_id, std::make_unique<drake::geometry::GeometryInstance>(
                             drake::math::RigidTransform<double>::Identity(),
                             std::make_unique<drake::geometry::Sphere>(kRadius),
                             ss.str()));
      scene_graph->AssignRole(source_id, geometry_id,
                              drake::geometry::IllustrationProperties());
    }
    return {};
  }

  static void CheckSceneMarkers(
      const visualization_msgs::msg::MarkerArray& marker_array,
      SceneMarkersSystem* scene_markers_system) {
    ASSERT_EQ(marker_array.markers.size(), 1u + kNumGeometries);
    const visualization_msgs::msg::Marker& control_marker =
        marker_array.markers[0];
    EXPECT_EQ(control_marker.action,
              visualization_msgs::msg::Marker::DELETEALL);

    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      const visualization_msgs::msg::Marker& marker =
          marker_array.markers[1 + i];
      EXPECT_EQ(marker.header.frame_id, "world");
      EXPECT_EQ(marker.header.stamp.sec, 0);
      EXPECT_EQ(marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(marker.ns, std::string(kSourceName));
      EXPECT_EQ(marker.id, static_cast<int>(i));
      EXPECT_EQ(marker.action, visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(marker.type, visualization_msgs::msg::Marker::SPHERE);
      EXPECT_EQ(marker.lifetime.sec, 0);
      EXPECT_EQ(marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(marker.frame_locked);
      constexpr double kDiameter = 2. * kRadius;
      EXPECT_DOUBLE_EQ(marker.scale.x, kDiameter);
      EXPECT_DOUBLE_EQ(marker.scale.y, kDiameter);
      EXPECT_DOUBLE_EQ(marker.scale.z, kDiameter);
      const drake::geometry::Rgba& default_color =
          scene_markers_system->params().default_color;
      EXPECT_NEAR(marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(marker.color.a, default_color.a(), kTolerance);
      EXPECT_DOUBLE_EQ(marker.pose.position.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.w, 1.);
    }
  }
};

struct SingleEllipsoidSceneTestDetails {
  static constexpr char kName[] = "ellipsoid";
  static constexpr double kLengthA{0.3};
  static constexpr double kLengthB{0.4};
  static constexpr double kLengthC{0.5};

  static drake::geometry::FramePoseVector<double> PopulateSceneGraph(
      const drake::geometry::SourceId& source_id,
      drake::geometry::SceneGraph<double>* scene_graph) {
    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      std::stringstream ss;
      ss << kName << i;
      const drake::geometry::GeometryId geometry_id =
          scene_graph->RegisterAnchoredGeometry(
              source_id, std::make_unique<drake::geometry::GeometryInstance>(
                             drake::math::RigidTransform<double>::Identity(),
                             std::make_unique<drake::geometry::Ellipsoid>(
                                 kLengthA, kLengthB, kLengthC),
                             ss.str()));
      scene_graph->AssignRole(source_id, geometry_id,
                              drake::geometry::IllustrationProperties());
    }
    return {};
  }

  static void CheckSceneMarkers(
      const visualization_msgs::msg::MarkerArray& marker_array,
      SceneMarkersSystem* scene_markers_system) {
    ASSERT_EQ(marker_array.markers.size(), 1u + kNumGeometries);
    const visualization_msgs::msg::Marker& control_marker =
        marker_array.markers[0];
    EXPECT_EQ(control_marker.action,
              visualization_msgs::msg::Marker::DELETEALL);

    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      const visualization_msgs::msg::Marker& marker =
          marker_array.markers[1 + i];
      EXPECT_EQ(marker.header.frame_id, "world");
      EXPECT_EQ(marker.header.stamp.sec, 0);
      EXPECT_EQ(marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(marker.ns, std::string(kSourceName));
      EXPECT_EQ(marker.id, static_cast<int>(i));
      EXPECT_EQ(marker.action, visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(marker.type, visualization_msgs::msg::Marker::SPHERE);
      EXPECT_EQ(marker.lifetime.sec, 0);
      EXPECT_EQ(marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(marker.frame_locked);
      EXPECT_DOUBLE_EQ(marker.scale.x, 2. * kLengthA);
      EXPECT_DOUBLE_EQ(marker.scale.y, 2. * kLengthB);
      EXPECT_DOUBLE_EQ(marker.scale.z, 2. * kLengthC);
      const drake::geometry::Rgba& default_color =
          scene_markers_system->params().default_color;
      EXPECT_NEAR(marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(marker.color.a, default_color.a(), kTolerance);
      EXPECT_DOUBLE_EQ(marker.pose.position.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.w, 1.);
    }
  }
};

struct SingleCylinderSceneTestDetails {
  static constexpr char kName[] = "cylinder";
  static constexpr double kRadius{0.5};
  static constexpr double kLength{1.0};

  static drake::geometry::FramePoseVector<double> PopulateSceneGraph(
      const drake::geometry::SourceId& source_id,
      drake::geometry::SceneGraph<double>* scene_graph) {
    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      std::stringstream ss;
      ss << kName << i;
      const drake::geometry::GeometryId geometry_id =
          scene_graph->RegisterAnchoredGeometry(
              source_id,
              std::make_unique<drake::geometry::GeometryInstance>(
                  drake::math::RigidTransform<double>::Identity(),
                  std::make_unique<drake::geometry::Cylinder>(kRadius, kLength),
                  ss.str()));
      scene_graph->AssignRole(source_id, geometry_id,
                              drake::geometry::IllustrationProperties());
    }
    return {};
  }

  static void CheckSceneMarkers(
      const visualization_msgs::msg::MarkerArray& marker_array,
      SceneMarkersSystem* scene_markers_system) {
    ASSERT_EQ(marker_array.markers.size(), 1u + kNumGeometries);
    const visualization_msgs::msg::Marker& control_marker =
        marker_array.markers[0];
    EXPECT_EQ(control_marker.action,
              visualization_msgs::msg::Marker::DELETEALL);

    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      const visualization_msgs::msg::Marker& marker =
          marker_array.markers[1 + i];
      EXPECT_EQ(marker.header.frame_id, "world");
      EXPECT_EQ(marker.header.stamp.sec, 0);
      EXPECT_EQ(marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(marker.ns, std::string(kSourceName));
      EXPECT_EQ(marker.id, static_cast<int>(i));
      EXPECT_EQ(marker.action, visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(marker.type, visualization_msgs::msg::Marker::CYLINDER);
      EXPECT_EQ(marker.lifetime.sec, 0);
      EXPECT_EQ(marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(marker.frame_locked);
      constexpr double kDiameter = 2. * kRadius;
      EXPECT_DOUBLE_EQ(marker.scale.x, kDiameter);
      EXPECT_DOUBLE_EQ(marker.scale.y, kDiameter);
      EXPECT_DOUBLE_EQ(marker.scale.z, kLength);
      const drake::geometry::Rgba& default_color =
          scene_markers_system->params().default_color;
      EXPECT_NEAR(marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(marker.color.a, default_color.a(), kTolerance);
      EXPECT_DOUBLE_EQ(marker.pose.position.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.w, 1.);
    }
  }
};

struct SingleHalfSpaceSceneTestDetails {
  static constexpr char kName[] = "hspace";

  static drake::geometry::FramePoseVector<double> PopulateSceneGraph(
      const drake::geometry::SourceId& source_id,
      drake::geometry::SceneGraph<double>* scene_graph) {
    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      std::stringstream ss;
      ss << kName << i;
      const drake::geometry::GeometryId geometry_id =
          scene_graph->RegisterAnchoredGeometry(
              source_id,
              std::make_unique<drake::geometry::GeometryInstance>(
                  drake::math::RigidTransform<double>::Identity(),
                  std::make_unique<drake::geometry::HalfSpace>(), ss.str()));
      scene_graph->AssignRole(source_id, geometry_id,
                              drake::geometry::IllustrationProperties());
    }
    return {};
  }

  static void CheckSceneMarkers(
      const visualization_msgs::msg::MarkerArray& marker_array,
      SceneMarkersSystem* scene_markers_system) {
    ASSERT_EQ(marker_array.markers.size(), 1u + kNumGeometries);
    const visualization_msgs::msg::Marker& control_marker =
        marker_array.markers[0];
    EXPECT_EQ(control_marker.action,
              visualization_msgs::msg::Marker::DELETEALL);

    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      const visualization_msgs::msg::Marker& marker =
          marker_array.markers[1 + i];
      EXPECT_EQ(marker.header.frame_id, "world");
      EXPECT_EQ(marker.header.stamp.sec, 0);
      EXPECT_EQ(marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(marker.ns, std::string(kSourceName));
      EXPECT_EQ(marker.id, static_cast<int>(i));
      EXPECT_EQ(marker.action, visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(marker.type, visualization_msgs::msg::Marker::CUBE);
      EXPECT_EQ(marker.lifetime.sec, 0);
      EXPECT_EQ(marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(marker.frame_locked);
      EXPECT_GT(marker.scale.x, 10.0);
      EXPECT_GT(marker.scale.y, 10.0);
      const drake::geometry::Rgba& default_color =
          scene_markers_system->params().default_color;
      EXPECT_NEAR(marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(marker.color.a, default_color.a(), kTolerance);
    }
  }
};

struct SingleBoxSceneTestDetails {
  static constexpr char kName[] = "box";
  static constexpr double kWidth{0.5};
  static constexpr double kDepth{0.25};
  static constexpr double kHeight{1.0};

  static drake::geometry::FramePoseVector<double> PopulateSceneGraph(
      const drake::geometry::SourceId& source_id,
      drake::geometry::SceneGraph<double>* scene_graph) {
    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      std::stringstream ss;
      ss << kName << i;
      const drake::geometry::GeometryId geometry_id =
          scene_graph->RegisterAnchoredGeometry(
              source_id, std::make_unique<drake::geometry::GeometryInstance>(
                             drake::math::RigidTransform<double>::Identity(),
                             std::make_unique<drake::geometry::Box>(
                                 kWidth, kDepth, kHeight),
                             ss.str()));
      scene_graph->AssignRole(source_id, geometry_id,
                              drake::geometry::IllustrationProperties());
    }
    return {};
  }

  static void CheckSceneMarkers(
      const visualization_msgs::msg::MarkerArray& marker_array,
      SceneMarkersSystem* scene_markers_system) {
    ASSERT_EQ(marker_array.markers.size(), 1u + kNumGeometries);
    const visualization_msgs::msg::Marker& control_marker =
        marker_array.markers[0];
    EXPECT_EQ(control_marker.action,
              visualization_msgs::msg::Marker::DELETEALL);

    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      const visualization_msgs::msg::Marker& marker =
          marker_array.markers[1 + i];
      EXPECT_EQ(marker.header.frame_id, "world");
      EXPECT_EQ(marker.header.stamp.sec, 0);
      EXPECT_EQ(marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(marker.ns, std::string(kSourceName));
      EXPECT_EQ(marker.id, static_cast<int>(i));
      EXPECT_EQ(marker.action, visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(marker.type, visualization_msgs::msg::Marker::CUBE);
      EXPECT_EQ(marker.lifetime.sec, 0);
      EXPECT_EQ(marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(marker.frame_locked);
      EXPECT_DOUBLE_EQ(marker.scale.x, kWidth);
      EXPECT_DOUBLE_EQ(marker.scale.y, kDepth);
      EXPECT_DOUBLE_EQ(marker.scale.z, kHeight);
      const drake::geometry::Rgba& default_color =
          scene_markers_system->params().default_color;
      EXPECT_NEAR(marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(marker.color.a, default_color.a(), kTolerance);
      EXPECT_DOUBLE_EQ(marker.pose.position.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.w, 1.);
    }
  }
};

struct SingleCapsuleSceneTestDetails {
  static constexpr char kName[] = "capsule";
  static constexpr double kRadius{0.25};
  static constexpr double kLength{0.5};

  static drake::geometry::FramePoseVector<double> PopulateSceneGraph(
      const drake::geometry::SourceId& source_id,
      drake::geometry::SceneGraph<double>* scene_graph) {
    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      std::stringstream ss;
      ss << kName << i;
      const drake::geometry::GeometryId geometry_id =
          scene_graph->RegisterAnchoredGeometry(
              source_id,
              std::make_unique<drake::geometry::GeometryInstance>(
                  drake::math::RigidTransform<double>::Identity(),
                  std::make_unique<drake::geometry::Capsule>(kRadius, kLength),
                  ss.str()));
      scene_graph->AssignRole(source_id, geometry_id,
                              drake::geometry::IllustrationProperties());
    }
    return {};
  }

  static void CheckSceneMarkers(
      const visualization_msgs::msg::MarkerArray& marker_array,
      SceneMarkersSystem* scene_markers_system) {
    ASSERT_EQ(marker_array.markers.size(), 1u + 3 * kNumGeometries);
    const visualization_msgs::msg::Marker& control_marker =
        marker_array.markers[0];
    EXPECT_EQ(control_marker.action,
              visualization_msgs::msg::Marker::DELETEALL);

    const drake::geometry::Rgba& default_color =
        scene_markers_system->params().default_color;

    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      const visualization_msgs::msg::Marker& body_marker =
          marker_array.markers[1 + i * 3];
      EXPECT_EQ(body_marker.header.frame_id, "world");
      EXPECT_EQ(body_marker.header.stamp.sec, 0);
      EXPECT_EQ(body_marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(body_marker.ns, std::string(kSourceName));
      EXPECT_EQ(body_marker.id, static_cast<int>(i * 3));
      EXPECT_EQ(body_marker.action, visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(body_marker.type, visualization_msgs::msg::Marker::CYLINDER);
      EXPECT_EQ(body_marker.lifetime.sec, 0);
      EXPECT_EQ(body_marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(body_marker.frame_locked);
      constexpr double kDiameter = 2. * kRadius;
      EXPECT_DOUBLE_EQ(body_marker.scale.x, kDiameter);
      EXPECT_DOUBLE_EQ(body_marker.scale.y, kDiameter);
      EXPECT_DOUBLE_EQ(body_marker.scale.z, kLength);
      EXPECT_NEAR(body_marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(body_marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(body_marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(body_marker.color.a, default_color.a(), kTolerance);
      EXPECT_DOUBLE_EQ(body_marker.pose.position.x, 0.);
      EXPECT_DOUBLE_EQ(body_marker.pose.position.y, 0.);
      EXPECT_DOUBLE_EQ(body_marker.pose.position.z, 0.);
      EXPECT_DOUBLE_EQ(body_marker.pose.orientation.x, 0.);
      EXPECT_DOUBLE_EQ(body_marker.pose.orientation.y, 0.);
      EXPECT_DOUBLE_EQ(body_marker.pose.orientation.z, 0.);
      EXPECT_DOUBLE_EQ(body_marker.pose.orientation.w, 1.);

      const visualization_msgs::msg::Marker& upper_cap_marker =
          marker_array.markers[1 + i * 3 + 1];
      EXPECT_EQ(upper_cap_marker.header.frame_id, "world");
      EXPECT_EQ(upper_cap_marker.header.stamp.sec, 0);
      EXPECT_EQ(upper_cap_marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(upper_cap_marker.ns, std::string(kSourceName));
      EXPECT_EQ(upper_cap_marker.id, static_cast<int>(i * 3 + 1));
      EXPECT_EQ(upper_cap_marker.action,
                visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(upper_cap_marker.type, visualization_msgs::msg::Marker::SPHERE);
      EXPECT_EQ(upper_cap_marker.lifetime.sec, 0);
      EXPECT_EQ(upper_cap_marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(upper_cap_marker.frame_locked);
      EXPECT_DOUBLE_EQ(upper_cap_marker.scale.x, kDiameter);
      EXPECT_DOUBLE_EQ(upper_cap_marker.scale.y, kDiameter);
      EXPECT_DOUBLE_EQ(upper_cap_marker.scale.z, kDiameter);
      EXPECT_NEAR(upper_cap_marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(upper_cap_marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(upper_cap_marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(upper_cap_marker.color.a, default_color.a(), kTolerance);
      EXPECT_DOUBLE_EQ(upper_cap_marker.pose.position.x, 0.);
      EXPECT_DOUBLE_EQ(upper_cap_marker.pose.position.y, 0.);
      EXPECT_DOUBLE_EQ(upper_cap_marker.pose.position.z, kLength / 2);
      EXPECT_DOUBLE_EQ(upper_cap_marker.pose.orientation.x, 0.);
      EXPECT_DOUBLE_EQ(upper_cap_marker.pose.orientation.y, 0.);
      EXPECT_DOUBLE_EQ(upper_cap_marker.pose.orientation.z, 0.);
      EXPECT_DOUBLE_EQ(upper_cap_marker.pose.orientation.w, 1.);

      const visualization_msgs::msg::Marker& lower_cap_marker =
          marker_array.markers[1 + i * 3 + 2];
      EXPECT_EQ(lower_cap_marker.header.frame_id, "world");
      EXPECT_EQ(lower_cap_marker.header.stamp.sec, 0);
      EXPECT_EQ(lower_cap_marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(lower_cap_marker.ns, std::string(kSourceName));
      EXPECT_EQ(lower_cap_marker.id, static_cast<int>(i * 3 + 2));
      EXPECT_EQ(lower_cap_marker.action,
                visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(lower_cap_marker.type, visualization_msgs::msg::Marker::SPHERE);
      EXPECT_EQ(lower_cap_marker.lifetime.sec, 0);
      EXPECT_EQ(lower_cap_marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(lower_cap_marker.frame_locked);
      EXPECT_DOUBLE_EQ(lower_cap_marker.scale.x, kDiameter);
      EXPECT_DOUBLE_EQ(lower_cap_marker.scale.y, kDiameter);
      EXPECT_DOUBLE_EQ(lower_cap_marker.scale.z, kDiameter);
      EXPECT_NEAR(lower_cap_marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(lower_cap_marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(lower_cap_marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(lower_cap_marker.color.a, default_color.a(), kTolerance);
      EXPECT_DOUBLE_EQ(lower_cap_marker.pose.position.x, 0.);
      EXPECT_DOUBLE_EQ(lower_cap_marker.pose.position.y, 0.);
      EXPECT_DOUBLE_EQ(lower_cap_marker.pose.position.z, -kLength / 2);
      EXPECT_DOUBLE_EQ(lower_cap_marker.pose.orientation.x, 0.);
      EXPECT_DOUBLE_EQ(lower_cap_marker.pose.orientation.y, 0.);
      EXPECT_DOUBLE_EQ(lower_cap_marker.pose.orientation.z, 0.);
      EXPECT_DOUBLE_EQ(lower_cap_marker.pose.orientation.w, 1.);
    }
  }
};

template <typename T>
struct SingleMeshSceneTestDetails {
  static constexpr char kName[] = "mesh";
  static constexpr char kFilename[] = "/tmp/dummy.obj";
  static constexpr double kScale{0.1};

  static drake::geometry::FramePoseVector<double> PopulateSceneGraph(
      const drake::geometry::SourceId& source_id,
      drake::geometry::SceneGraph<double>* scene_graph) {
    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      std::stringstream ss;
      ss << kName << i;
      const drake::geometry::GeometryId geometry_id =
          scene_graph->RegisterAnchoredGeometry(
              source_id, std::make_unique<drake::geometry::GeometryInstance>(
                             drake::math::RigidTransform<double>::Identity(),
                             std::make_unique<T>(kFilename, kScale), ss.str()));
      scene_graph->AssignRole(source_id, geometry_id,
                              drake::geometry::IllustrationProperties());
    }
    return {};
  }

  static void CheckSceneMarkers(
      const visualization_msgs::msg::MarkerArray& marker_array,
      SceneMarkersSystem* scene_markers_system) {
    ASSERT_EQ(marker_array.markers.size(), 1u + kNumGeometries);
    const visualization_msgs::msg::Marker& control_marker =
        marker_array.markers[0];
    EXPECT_EQ(control_marker.action,
              visualization_msgs::msg::Marker::DELETEALL);

    for (unsigned int i = 0; i < kNumGeometries; ++i) {
      const visualization_msgs::msg::Marker& marker =
          marker_array.markers[1 + i];
      EXPECT_EQ(marker.header.frame_id, "world");
      EXPECT_EQ(marker.header.stamp.sec, 0);
      EXPECT_EQ(marker.header.stamp.nanosec, 0u);
      EXPECT_EQ(marker.ns, std::string(kSourceName));
      EXPECT_EQ(marker.id, static_cast<int>(i));
      EXPECT_EQ(marker.action, visualization_msgs::msg::Marker::MODIFY);
      EXPECT_EQ(marker.type, visualization_msgs::msg::Marker::MESH_RESOURCE);
      EXPECT_EQ(marker.lifetime.sec, 0);
      EXPECT_EQ(marker.lifetime.nanosec, 0u);
      EXPECT_TRUE(marker.frame_locked);
      EXPECT_EQ(marker.mesh_resource, std::string("file://") + kFilename);
      EXPECT_DOUBLE_EQ(marker.scale.x, kScale);
      EXPECT_DOUBLE_EQ(marker.scale.y, kScale);
      EXPECT_DOUBLE_EQ(marker.scale.z, kScale);
      const drake::geometry::Rgba& default_color =
          scene_markers_system->params().default_color;
      EXPECT_NEAR(marker.color.r, default_color.r(), kTolerance);
      EXPECT_NEAR(marker.color.g, default_color.g(), kTolerance);
      EXPECT_NEAR(marker.color.b, default_color.b(), kTolerance);
      EXPECT_NEAR(marker.color.a, default_color.a(), kTolerance);
      EXPECT_DOUBLE_EQ(marker.pose.position.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.position.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.x, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.y, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.z, 0.);
      EXPECT_DOUBLE_EQ(marker.pose.orientation.w, 1.);
    }
  }
};

template <typename T>
class SceneMarkersTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(SceneMarkersTest);

TYPED_TEST_P(SceneMarkersTest, NominalCase) {
  using TestDetails = TypeParam;

  drake::systems::DiagramBuilder<double> builder;
  auto scene_graph = builder.AddSystem<drake::geometry::SceneGraph>();
  auto source_id = scene_graph->RegisterSource(kSourceName);
  auto pose_vector_value = drake::AbstractValue::Make(
      TestDetails::PopulateSceneGraph(source_id, scene_graph));
  auto pose_vector_source =
      builder.AddSystem<drake::systems::ConstantValueSource>(
          *pose_vector_value);
  builder.Connect(pose_vector_source->get_output_port(),
                  scene_graph->get_source_pose_port(source_id));

  auto scene_markers = builder.AddSystem<SceneMarkersSystem>();
  builder.Connect(scene_graph->get_query_output_port(),
                  scene_markers->get_graph_query_port());

  builder.ExportOutput(scene_markers->get_markers_output_port());

  std::unique_ptr<drake::systems::Diagram<double>> diagram = builder.Build();
  std::unique_ptr<drake::systems::Context<double>> context =
      diagram->CreateDefaultContext();

  const drake::systems::OutputPort<double>& markers_port =
      diagram->get_output_port();
  auto marker_array =
      markers_port.Eval<visualization_msgs::msg::MarkerArray>(*context);

  TestDetails::CheckSceneMarkers(marker_array, scene_markers);
}

REGISTER_TYPED_TEST_SUITE_P(SceneMarkersTest, NominalCase);

using SingleGeometrySceneTestDetails = ::testing::Types<
    SingleSphereSceneTestDetails, SingleEllipsoidSceneTestDetails,
    SingleCylinderSceneTestDetails, SingleHalfSpaceSceneTestDetails,
    SingleBoxSceneTestDetails, SingleCapsuleSceneTestDetails,
    SingleMeshSceneTestDetails<drake::geometry::Convex>,
    SingleMeshSceneTestDetails<drake::geometry::Mesh>>;

INSTANTIATE_TYPED_TEST_SUITE_P(SingleGeometrySceneMarkersTests,
                               SceneMarkersTest,
                               SingleGeometrySceneTestDetails);
