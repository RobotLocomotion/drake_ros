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

#include "drake_ros_viz/scene_markers_system.hpp"

#include <unordered_set>
#include <utility>

#include "drake_ros_viz/utilities/name_conventions.hpp"
#include "drake_ros_viz/utilities/type_conversion.hpp"
#include <builtin_interfaces/msg/time.hpp>
#include <drake/common/drake_copyable.h>
#include <drake/common/eigen_types.h>
#include <drake/geometry/geometry_properties.h>
#include <drake/geometry/geometry_roles.h>
#include <drake/geometry/query_object.h>
#include <drake/geometry/scene_graph.h>
#include <drake/geometry/scene_graph_inspector.h>
#include <drake/geometry/shape_specification.h>
#include <drake/math/rigid_transform.h>
#include <drake/systems/framework/leaf_system.h>
#include <drake_ros_tf2/utilities/name_conventions.hpp>
#include <rclcpp/duration.hpp>
#include <rclcpp/time.hpp>
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

namespace drake_ros_viz {

namespace {

class SceneGeometryToMarkers : public drake::geometry::ShapeReifier {
 public:
  DRAKE_NO_COPY_NO_MOVE_NO_ASSIGN(SceneGeometryToMarkers)

  explicit SceneGeometryToMarkers(const SceneMarkersParams& params)
      : params_(params) {}

  ~SceneGeometryToMarkers() override = default;

  void Populate(
      const drake::geometry::SceneGraphInspector<double>& inspector,
      const std::unordered_set<const drake::multibody::MultibodyPlant<double>*>
          plants,
      const drake::geometry::GeometryId& geometry_id,
      visualization_msgs::msg::MarkerArray* marker_array) {
    DRAKE_ASSERT(nullptr != marker_array);
    marker_array_ = marker_array;

    prototype_marker_.header.frame_id =
        drake_ros_tf2::utilities::GetTfFrameName(inspector, plants,
                                                 geometry_id);
    prototype_marker_.ns = params_.marker_namespace.value_or(
        utilities::GetMarkerNamespace(inspector, plants, geometry_id));
    prototype_marker_.id = marker_array_->markers.size();
    prototype_marker_.action = visualization_msgs::msg::Marker::MODIFY;

    prototype_marker_.lifetime = rclcpp::Duration::from_nanoseconds(0);
    prototype_marker_.frame_locked = true;

    const drake::geometry::GeometryProperties* props =
        inspector.GetProperties(geometry_id, params_.role);
    DRAKE_ASSERT(nullptr != props);
    drake::geometry::Rgba default_color = params_.default_color;
    if (drake::geometry::Role::kIllustration != params_.role) {
      const drake::geometry::IllustrationProperties* illustration_props =
          inspector.GetIllustrationProperties(geometry_id);
      if (illustration_props) {
        default_color = illustration_props->GetPropertyOrDefault(
            "phong", "diffuse", default_color);
      }
    }
    const drake::geometry::Rgba& color =
        props->GetPropertyOrDefault("phong", "diffuse", default_color);
    prototype_marker_.color.r = color.r();
    prototype_marker_.color.g = color.g();
    prototype_marker_.color.b = color.b();
    prototype_marker_.color.a = color.a();

    X_FG_ = inspector.GetPoseInFrame(geometry_id);

    inspector.GetShape(geometry_id).Reify(this);
  }

 private:
  using drake::geometry::ShapeReifier::ImplementGeometry;

  void ImplementGeometry(const drake::geometry::Sphere& sphere,
                         void*) override {
    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& marker = marker_array_->markers.back();
    marker.type = visualization_msgs::msg::Marker::SPHERE;
    const double diameter = 2. * sphere.radius();
    marker.scale.x = diameter;
    marker.scale.y = diameter;
    marker.scale.z = diameter;
    marker.pose = utilities::ToPoseMsg(X_FG_);
  }

  void ImplementGeometry(const drake::geometry::Ellipsoid& ellipsoid,
                         void*) override {
    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& marker = marker_array_->markers.back();
    marker.type = visualization_msgs::msg::Marker::SPHERE;
    marker.scale.x = 2. * ellipsoid.a();
    marker.scale.y = 2. * ellipsoid.b();
    marker.scale.z = 2. * ellipsoid.c();
    marker.pose = utilities::ToPoseMsg(X_FG_);
  }

  void ImplementGeometry(const drake::geometry::Cylinder& cylinder,
                         void*) override {
    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& marker = marker_array_->markers.back();
    marker.type = visualization_msgs::msg::Marker::CYLINDER;
    const double diameter = 2. * cylinder.radius();
    marker.scale.x = diameter;
    marker.scale.y = diameter;
    marker.scale.z = cylinder.length();
    marker.pose = utilities::ToPoseMsg(X_FG_);
  }

  void ImplementGeometry(const drake::geometry::HalfSpace&, void*) override {
    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& marker = marker_array_->markers.back();
    marker.type = visualization_msgs::msg::Marker::CUBE;
    constexpr double kHalfSpaceLength = 50.;
    constexpr double kHalfSpaceThickness = 1.;
    marker.scale.x = kHalfSpaceLength;
    marker.scale.y = kHalfSpaceLength;
    marker.scale.z = kHalfSpaceThickness;
    const drake::math::RigidTransform<double> X_GH{
        drake::Vector3<double>{0., 0., -kHalfSpaceThickness / 2.}};
    const drake::math::RigidTransform<double> X_FH = X_FG_ * X_GH;
    marker.pose = utilities::ToPoseMsg(X_FH);
  }

  void ImplementGeometry(const drake::geometry::Box& box, void*) override {
    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& marker = marker_array_->markers.back();
    marker.type = visualization_msgs::msg::Marker::CUBE;
    marker.scale.x = box.width();
    marker.scale.y = box.depth();
    marker.scale.z = box.height();
    marker.pose = utilities::ToPoseMsg(X_FG_);
  }

  void ImplementGeometry(const drake::geometry::Capsule& capsule,
                         void*) override {
    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& body_marker =
        marker_array_->markers.back();
    body_marker.type = visualization_msgs::msg::Marker::CYLINDER;
    const double diameter = 2. * capsule.radius();
    body_marker.scale.x = diameter;
    body_marker.scale.y = diameter;
    body_marker.scale.z = capsule.length();
    body_marker.pose = utilities::ToPoseMsg(X_FG_);

    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& upper_cap_marker =
        marker_array_->markers.back();
    upper_cap_marker.id = body_marker.id + 1;
    upper_cap_marker.type = visualization_msgs::msg::Marker::SPHERE;
    upper_cap_marker.scale.x = diameter;
    upper_cap_marker.scale.y = diameter;
    upper_cap_marker.scale.z = diameter;
    const drake::math::RigidTransform<double> X_GU{
        drake::Vector3<double>{0., 0., capsule.length() / 2.}};
    const drake::math::RigidTransform<double> X_FU = X_FG_ * X_GU;
    upper_cap_marker.pose = utilities::ToPoseMsg(X_FU);

    marker_array_->markers.push_back(upper_cap_marker);

    visualization_msgs::msg::Marker& lower_cap_marker =
        marker_array_->markers.back();
    lower_cap_marker.id = upper_cap_marker.id + 1;
    const drake::math::RigidTransform<double> X_GL{
        drake::Vector3<double>{0., 0., -capsule.length() / 2.}};
    const drake::math::RigidTransform<double> X_FL = X_FG_ * X_GL;
    lower_cap_marker.pose = utilities::ToPoseMsg(X_FL);
  }

  void ImplementGeometry(const drake::geometry::Convex& convex,
                         void*) override {
    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& marker = marker_array_->markers.back();
    marker.type = visualization_msgs::msg::Marker::MESH_RESOURCE;
    marker.scale.x = convex.scale();
    marker.scale.y = convex.scale();
    marker.scale.z = convex.scale();
    // Assume it is an absolute path and turn it into a file URL.
    marker.mesh_resource = "file://" + convex.filename();
    marker.pose = utilities::ToPoseMsg(X_FG_);
  }

  void ImplementGeometry(const drake::geometry::Mesh& mesh, void*) override {
    marker_array_->markers.push_back(prototype_marker_);

    visualization_msgs::msg::Marker& marker = marker_array_->markers.back();
    marker.type = visualization_msgs::msg::Marker::MESH_RESOURCE;
    marker.scale.x = mesh.scale();
    marker.scale.y = mesh.scale();
    marker.scale.z = mesh.scale();
    // Assume it is an absolute path and turn it into a file URL.
    marker.mesh_resource = "file://" + mesh.filename();
    marker.pose = utilities::ToPoseMsg(X_FG_);
  }

  const SceneMarkersParams& params_;
  visualization_msgs::msg::MarkerArray* marker_array_{nullptr};
  visualization_msgs::msg::Marker prototype_marker_{};
  drake::math::RigidTransform<double> X_FG_{};
};

}  // namespace

class SceneMarkersSystem::SceneMarkersSystemPrivate {
 public:
  explicit SceneMarkersSystemPrivate(SceneMarkersParams _params)
      : params(std::move(_params)) {}

  const SceneMarkersParams params;
  drake::systems::CacheIndex scene_markers_cache_index;
  drake::systems::InputPortIndex graph_query_port_index;
  drake::systems::OutputPortIndex scene_markers_port_index;
  std::unordered_set<const drake::multibody::MultibodyPlant<double>*> plants;
  mutable drake::geometry::GeometryVersion version;
};

SceneMarkersSystem::SceneMarkersSystem(SceneMarkersParams params)
    : impl_(new SceneMarkersSystemPrivate(std::move(params))) {
  impl_->graph_query_port_index =
      this->DeclareAbstractInputPort(
              "graph_query",
              drake::Value<drake::geometry::QueryObject<double>>{})
          .get_index();

  impl_->scene_markers_cache_index =
      this->DeclareCacheEntry("scene_markers_cache",
                              &SceneMarkersSystem::CalcSceneMarkers,
                              {nothing_ticket()})
          .cache_index();

  impl_->scene_markers_port_index =
      this->DeclareAbstractOutputPort(
              "scene_markers", &SceneMarkersSystem::PopulateSceneMarkersMessage)
          .get_index();
}

SceneMarkersSystem::~SceneMarkersSystem() {}

void SceneMarkersSystem::RegisterMultibodyPlant(
    const drake::multibody::MultibodyPlant<double>* plant) {
  DRAKE_THROW_UNLESS(plant != nullptr);
  impl_->plants.insert(plant);
}

namespace {

visualization_msgs::msg::Marker MakeDeleteAllMarker() {
  visualization_msgs::msg::Marker marker;
  marker.action = visualization_msgs::msg::Marker::DELETEALL;
  return marker;
}

}  // namespace

void SceneMarkersSystem::PopulateSceneMarkersMessage(
    const drake::systems::Context<double>& context,
    visualization_msgs::msg::MarkerArray* output_value) const {
  bool cached;
  *output_value = this->EvalSceneMarkers(context, &cached);
  if (!cached) {
    // Cache invalidated after scene change.
    // Delete all pre-existing markers before an update.
    output_value->markers.insert(output_value->markers.begin(),
                                 MakeDeleteAllMarker());
  }
  const builtin_interfaces::msg::Time stamp =
      rclcpp::Time() + rclcpp::Duration::from_seconds(context.get_time());
  for (visualization_msgs::msg::Marker& marker : output_value->markers) {
    marker.header.stamp = stamp;
  }
}

const visualization_msgs::msg::MarkerArray&
SceneMarkersSystem::EvalSceneMarkers(
    const drake::systems::Context<double>& context, bool* cached) const {
  const drake::geometry::QueryObject<double>& query_object =
      get_input_port(impl_->graph_query_port_index)
          .Eval<drake::geometry::QueryObject<double>>(context);
  const drake::geometry::GeometryVersion& current_version =
      query_object.inspector().geometry_version();
  const bool same_version =
      impl_->version.IsSameAs(current_version, impl_->params.role);
  if (!same_version) {
    // Invalidate scene markers cache
    get_cache_entry(impl_->scene_markers_cache_index)
        .get_mutable_cache_entry_value(context)
        .mark_out_of_date();
    impl_->version = current_version;
  }
  if (cached) {
    *cached = same_version;
  }
  return get_cache_entry(impl_->scene_markers_cache_index)
      .Eval<visualization_msgs::msg::MarkerArray>(context);
}

void SceneMarkersSystem::CalcSceneMarkers(
    const drake::systems::Context<double>& context,
    visualization_msgs::msg::MarkerArray* output_value) const {
  const drake::geometry::QueryObject<double>& query_object =
      get_input_port(impl_->graph_query_port_index)
          .Eval<drake::geometry::QueryObject<double>>(context);
  const drake::geometry::SceneGraphInspector<double>& inspector =
      query_object.inspector();
  output_value->markers.reserve(
      inspector.NumGeometriesWithRole(impl_->params.role));
  for (const drake::geometry::FrameId& frame_id : inspector.GetAllFrameIds()) {
    for (const drake::geometry::GeometryId& geometry_id :
         inspector.GetGeometries(frame_id, impl_->params.role)) {
      SceneGeometryToMarkers(impl_->params)
          .Populate(inspector, impl_->plants, geometry_id, output_value);
    }
  }
}

const SceneMarkersParams& SceneMarkersSystem::params() const {
  return impl_->params;
}

const drake::systems::InputPort<double>&
SceneMarkersSystem::get_graph_query_port() const {
  return get_input_port(impl_->graph_query_port_index);
}

const drake::systems::OutputPort<double>&
SceneMarkersSystem::get_markers_output_port() const {
  return get_output_port(impl_->scene_markers_port_index);
}

}  // namespace drake_ros_viz
