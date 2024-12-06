#include <memory>
#include <unordered_set>

#include <drake/systems/framework/leaf_system.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <rclcpp/qos.hpp>

#include "drake_ros/core/camera_info_system.h"
#include "drake_ros/core/clock_system.h"
#include "drake_ros/core/drake_ros.h"
#include "drake_ros/core/geometry_conversions.h"
#include "drake_ros/core/geometry_conversions_pybind.h"
#include "drake_ros/core/qos_pybind.h"
#include "drake_ros/core/image_system.h"
#include "drake_ros/core/ros_interface_system.h"
#include "drake_ros/core/ros_publisher_system.h"
#include "drake_ros/core/ros_subscriber_system.h"
#include "drake_ros/core/serializer_interface.h"
#include "drake_ros/drake_ros_pybind.h"

namespace drake_ros {
namespace drake_ros_py DRAKE_ROS_NO_EXPORT {

using drake_ros::core::CameraInfoSystem;
using drake_ros::core::ClockSystem;
using drake_ros::core::DrakeRos;
using drake_ros::core::init;
using drake_ros::core::ImageSystem;
using drake_ros::core::RosInterfaceSystem;
using drake_ros::core::RosPublisherSystem;
using drake_ros::core::RosSubscriberSystem;
using drake_ros::core::SerializerInterface;
using drake_ros::core::shutdown;

using drake::systems::LeafSystem;
using drake::systems::TriggerType;

using py_rvp = pybind11::return_value_policy;

// Implementation for `overload_cast_explicit`. We must use this structure so
// that we can constrain what is inferred. Otherwise, the ambiguity confuses
// the compiler.
template <typename Return, typename... Args>
struct overload_cast_impl {
  auto operator()(Return (*func)(Args...)) const { return func; }

  template <typename Class>
  auto operator()(Return (Class::*method)(Args...)) const {
    return method;
  }

  template <typename Class>
  auto operator()(Return (Class::*method)(Args...) const) const {
    return method;
  }
};

/// Provides option to provide explicit signature when
/// `py::overload_cast<Args...>` fails to infer the Return argument.
template <typename Return, typename... Args>
constexpr auto overload_cast_explicit = overload_cast_impl<Return, Args...>{};

// A (de)serialization interface implementation for Python ROS messages
// that can be overriden from Python itself.
class PySerializerInterface : public py::wrapper<SerializerInterface> {
 public:
  using Base = py::wrapper<SerializerInterface>;

  PySerializerInterface() : Base() {}

  const rosidl_message_type_support_t* GetTypeSupport() const override {
    auto overload = [&]() -> py::capsule {
      PYBIND11_OVERLOAD_PURE(py::capsule, SerializerInterface, GetTypeSupport);
    };
    return static_cast<rosidl_message_type_support_t*>(overload());
  }

  std::unique_ptr<drake::AbstractValue> CreateDefaultValue() const override {
    PYBIND11_OVERLOAD_PURE(std::unique_ptr<drake::AbstractValue>,
                           SerializerInterface, CreateDefaultValue);
  }

  rclcpp::SerializedMessage Serialize(
      const drake::AbstractValue& abstract_value) const override {
    py::gil_scoped_acquire guard;
    auto overload = [&]() -> py::bytes {
      PYBIND11_OVERLOAD_PURE(py::bytes, SerializerInterface, Serialize,
                             &abstract_value);
    };
    std::string bytes = overload();
    rclcpp::SerializedMessage serialized_message(bytes.size());
    rcl_serialized_message_t& rcl_serialized_message =
        serialized_message.get_rcl_serialized_message();
    std::copy(bytes.data(), bytes.data() + bytes.size(),
              rcl_serialized_message.buffer);
    rcl_serialized_message.buffer_length = bytes.size();
    return serialized_message;
  }

  void Deserialize(const rclcpp::SerializedMessage& serialized_message,
                   drake::AbstractValue* abstract_value) const override {
    py::gil_scoped_acquire guard;
    const rcl_serialized_message_t& rcl_serialized_message =
        serialized_message.get_rcl_serialized_message();
    py::bytes serialized_message_bytes(
        reinterpret_cast<const char*>(rcl_serialized_message.buffer),
        rcl_serialized_message.buffer_length);
    PYBIND11_OVERLOAD_PURE(void, SerializerInterface, Deserialize,
                           serialized_message_bytes, abstract_value);
  }
};

void DefCore(py::module m) {
  m.doc() = "Python bindings for drake_ros.core";

  py::module::import("numpy");
  py::module::import("pydrake.systems.framework");
  py::module::import("pydrake.multibody.plant");
  py::module::import("pydrake.math");
  py::module::import("pydrake.common.eigen_geometry");

  {
    using Class = rclcpp::NodeOptions;
    py::class_<Class>(m, "CppNodeOptions", py::module_local())
        .def(ParamInit<Class>())
        .def_property(
            "arguments", [](const Class& self) { return self.arguments(); },
            [](Class* self, const std::vector<std::string>& value) {
              self->arguments(value);
            })
        .def_property(
            "use_global_arguments",
            [](const Class& self) { return self.use_global_arguments(); },
            [](Class* self, bool value) { self->use_global_arguments(value); })
        .def_property(
            "enable_rosout",
            [](const Class& self) { return self.enable_rosout(); },
            [](Class* self, bool value) { self->enable_rosout(value); })
        .def_property(
            "use_intra_process_comms",
            [](const Class& self) { return self.use_intra_process_comms(); },
            [](Class* self, bool value) {
              self->use_intra_process_comms(value);
            })
        .def_property(
            "start_parameter_event_publisher",
            [](const Class& self) {
              return self.start_parameter_event_publisher();
            },
            [](Class* self, bool value) {
              self->start_parameter_event_publisher(value);
            })
        .def_property(
            "use_clock_thread",
            [](const Class& self) { return self.use_clock_thread(); },
            [](Class* self, bool value) { self->use_clock_thread(value); })
        .def_property(
            "allow_undeclared_parameters",
            [](const Class& self) {
              return self.allow_undeclared_parameters();
            },
            [](Class* self, bool value) {
              self->allow_undeclared_parameters(value);
            })
        .def_property(
            "automatically_declare_parameters_from_overrides",
            [](const Class& self) {
              return self.automatically_declare_parameters_from_overrides();
            },
            [](Class* self, bool value) {
              self->automatically_declare_parameters_from_overrides(value);
            });
  }

  // N.B. We purposefully do not make this `py::module_local()`, as it seems to
  // cause a segfault for downstream bindings, at least for the
  // RobotLocomotion/pybind11 fork.
  py::class_<rclcpp::Node, std::shared_ptr<rclcpp::Node>>(m, "CppNode")
      .def(py::init<const std::string&, rclcpp::NodeOptions>(),
           py::arg("node_name"),
           py::arg("node_options") = rclcpp::NodeOptions{})
      .def("get_name", &rclcpp::Node::get_name);

  // TODD(hidmic): populate Python docstrings with
  // C++ docstrings. Consider using mkdoc to keep
  // them in sync, like pydrake does.
  py::class_<DrakeRos>(m, "DrakeRos")
      .def(py::init<const std::string&, rclcpp::NodeOptions>(),
           py::arg("node_name"),
           py::arg("node_options") = rclcpp::NodeOptions{})
      .def("get_node", [](const DrakeRos& self) {
        return self.get_node().shared_from_this();
      });

  py::class_<CameraInfoSystem, LeafSystem<double>>(m, "CameraInfoSystem")
      .def_static(
          "AddToBuilder",
          [](drake::systems::DiagramBuilder<double>* builder, DrakeRos* ros,
             const std::string& topic_name, const QoS& qos,
             const std::unordered_set<drake::systems::TriggerType>&
                 pub_triggers,
             double publish_period) {
            auto [camera_info_system, pub_system] =
                CameraInfoSystem::AddToBuilder(builder, ros, topic_name, qos,
                                               pub_triggers, publish_period);

            py::object py_builder = py::cast(builder, py_rvp::reference);
            py::list result;
            result.append(py::cast(camera_info_system,
                                   py_rvp::reference_internal, py_builder));
            result.append(
                py::cast(pub_system, py_rvp::reference_internal, py_builder));
            return result;
          },
          py::arg("builder"), py::arg("ros"), py::kw_only(),
          py::arg("topic_name") = std::string{"/image/camera_info"},
          py::arg("qos") = drake_ros::QoS(rclcpp::SensorDataQoS()),
          py::arg("publish_triggers") =
              std::unordered_set<drake::systems::TriggerType>{
                  RosPublisherSystem::kDefaultTriggerTypes},
          py::arg("publish_period") = 0.0)
      .def("set_camera_info", &CameraInfoSystem::SetCameraInfo);

  py::class_<ClockSystem, LeafSystem<double>>(m, "ClockSystem")
      .def_static(
          "AddToBuilder",
          [](drake::systems::DiagramBuilder<double>* builder, DrakeRos* ros,
             const std::string& topic_name, const QoS& qos,
             const std::unordered_set<drake::systems::TriggerType>&
                 pub_triggers,
             double publish_period) {
            auto [clock_system, pub_system] = ClockSystem::AddToBuilder(
                builder, ros, topic_name, qos, pub_triggers, publish_period);

            py::object py_builder = py::cast(builder, py_rvp::reference);
            py::list result;
            result.append(
                py::cast(clock_system, py_rvp::reference_internal, py_builder));
            result.append(
                py::cast(pub_system, py_rvp::reference_internal, py_builder));
            return result;
          },
          py::arg("builder"), py::arg("ros"), py::kw_only(),
          py::arg("topic_name") = std::string{"/clock"},
          py::arg("qos") = drake_ros::QoS(rclcpp::ClockQoS()),
          py::arg("publish_triggers") =
              std::unordered_set<drake::systems::TriggerType>{
                  RosPublisherSystem::kDefaultTriggerTypes},
          py::arg("publish_period") = 0.0);

  py::class_<ImageSystem, LeafSystem<double>>(m, "ImageSystem")
      .def(py::init<>())
      .def(
          "DeclareImageInputPort",
          [](ImageSystem& self, drake::systems::sensors::PixelType pixel_type,
             std::string port_name, double publish_period,
             double start_time) -> const drake::systems::InputPort<double>& {
            return self.DeclareImageInputPort(pixel_type, std::move(port_name),
                                              publish_period, start_time);
          },
          py::arg("pixel_type"), py::arg("port_name"),
          py::arg("publish_period"), py::arg("start_time"),
          py_rvp::reference_internal)
      .def(
          "DeclareDepthInputPort",
          [](ImageSystem& self, drake::systems::sensors::PixelType pixel_type,
             std::string port_name, double publish_period,
             double start_time) -> const drake::systems::InputPort<double>& {
            return self.DeclareDepthInputPort(pixel_type, std::move(port_name),
                                              publish_period, start_time);
          },
          py::arg("pixel_type"), py::arg("port_name"),
          py::arg("publish_period"), py::arg("start_time"),
          py_rvp::reference_internal)
      .def_static(
          "AddToBuilder",
          [](drake::systems::DiagramBuilder<double>* builder, DrakeRos* ros,
             const std::string& topic_name, const std::string& depth_topic_name,
             const QoS& qos,
             const std::unordered_set<drake::systems::TriggerType>&
                 pub_triggers,
             double publish_period) {
            auto [rgba_system, pub_system] = ImageSystem::AddToBuilder(
                builder, ros, topic_name, depth_topic_name, qos, pub_triggers,
                publish_period);

            py::object py_builder = py::cast(builder, py_rvp::reference);
            py::list result;
            result.append(
                py::cast(rgba_system, py_rvp::reference_internal, py_builder));
            result.append(
                py::cast(pub_system, py_rvp::reference_internal, py_builder));
            return result;
          },
          py::arg("builder"), py::arg("ros"), py::kw_only(),
          py::arg("topic_name") = std::string{"/image"},
          py::arg("depth_topic_name") = std::string{"/depth"},
          py::arg("qos") = drake_ros::QoS(rclcpp::SystemDefaultsQoS()),
          py::arg("publish_triggers") =
              std::unordered_set<drake::systems::TriggerType>{
                  RosPublisherSystem::kDefaultTriggerTypes},
          py::arg("publish_period") = 0.0);

  m.def(
      "init",
      [](std::vector<std::string> args) {
        std::vector<const char*> raw_args;
        raw_args.reserve(args.size());
        for (auto& arg : args) {
          raw_args.push_back(arg.c_str());
        }
        init(raw_args.size(), raw_args.data());
      },
      py::arg("args") = std::vector<std::string>{});
  m.def("shutdown", &shutdown);

  py::class_<RosInterfaceSystem, LeafSystem<double>>(m, "RosInterfaceSystem")
      .def(py::init([](const std::string& node_name) {
        return std::make_unique<RosInterfaceSystem>(
            std::make_unique<DrakeRos>(node_name));
      }))
      .def("get_ros_interface", &RosInterfaceSystem::get_ros_interface,
           py::return_value_policy::reference_internal);

  py::class_<SerializerInterface, PySerializerInterface>(m,
                                                         "SerializerInterface")
      .def(py::init([]() { return std::make_unique<PySerializerInterface>(); }))
      .def("CreateDefaultValue", &SerializerInterface::CreateDefaultValue)
      .def("GetTypeSupport",
           [](const SerializerInterface& self) {
             return py::capsule(self.GetTypeSupport());
           })
      .def("Serialize",
           [](const SerializerInterface& self,
              const drake::AbstractValue& abstract_value) {
             rclcpp::SerializedMessage serialized_message =
                 self.Serialize(abstract_value);
             const rcl_serialized_message_t& rcl_serialized_message =
                 serialized_message.get_rcl_serialized_message();
             return py::bytes(
                 reinterpret_cast<const char*>(rcl_serialized_message.buffer),
                 rcl_serialized_message.buffer_length);
           })
      .def("Deserialize",
           [](const SerializerInterface& self, py::bytes raw_bytes,
              drake::AbstractValue* abstract_value) {
             std::string bytes = raw_bytes;
             rclcpp::SerializedMessage serialized_message(bytes.size());
             rcl_serialized_message_t& rcl_serialized_message =
                 serialized_message.get_rcl_serialized_message();
             std::copy(bytes.data(), bytes.data() + bytes.size(),
                       rcl_serialized_message.buffer);
             rcl_serialized_message.buffer_length = bytes.size();
             self.Deserialize(serialized_message, abstract_value);
           });

  py::class_<RosPublisherSystem, LeafSystem<double>>(m, "RosPublisherSystem")
      .def(py::init([](std::unique_ptr<SerializerInterface> serializer,
                       const char* topic_name, QoS qos, DrakeRos* ros_interface,
                       std::unordered_set<TriggerType> publish_triggers,
                       double publish_period) {
        return std::make_unique<RosPublisherSystem>(
            std::move(serializer), topic_name, qos, ros_interface,
            publish_triggers, publish_period);
      }));

  py::class_<RosSubscriberSystem, LeafSystem<double>>(m, "RosSubscriberSystem")
      .def(py::init([](std::unique_ptr<SerializerInterface> serializer,
                       const char* topic_name, QoS qos,
                       DrakeRos* ros_interface) {
        return std::make_unique<RosSubscriberSystem>(
            std::move(serializer), topic_name, qos, ros_interface);
      }));

  // Python bindings for geometry conversions.
  // Vector / Translation functions.
  m.def("RosPointToVector3", &drake_ros::core::RosPointToVector3,
        py::arg("point"));
  m.def("Vector3ToRosPoint", &drake_ros::core::Vector3ToRosPoint,
        py::arg("point"));
  m.def("RosVector3ToVector3", &drake_ros::core::RosVector3ToVector3,
        py::arg("point"));
  m.def("Vector3ToRosVector3", &drake_ros::core::Vector3ToRosVector3,
        py::arg("point"));

  // Orientation
  m.def("RosQuaternionToQuaternion",
        &drake_ros::core::RosQuaternionToQuaternion, py::arg("quat"));
  m.def("QuaternionToRosQuaternion",
        &drake_ros::core::QuaternionToRosQuaternion, py::arg("quat"));
  m.def("RosQuaternionToRotationMatrix",
        &drake_ros::core::RosQuaternionToRotationMatrix, py::arg("quat"));
  m.def("RotationMatrixToRosQuaternion",
        &drake_ros::core::RotationMatrixToRosQuaternion, py::arg("rotation"));

  // Pose
  m.def("RosPoseToRigidTransform", &drake_ros::core::RosPoseToRigidTransform,
        py::arg("pose"));
  m.def("RigidTransformToRosPose", &drake_ros::core::RigidTransformToRosPose,
        py::arg("transform"));
  m.def("RosTransformToRigidTransform",
        &drake_ros::core::RosTransformToRigidTransform, py::arg("transform"));
  m.def("RigidTransformToRosTransform",
        &drake_ros::core::RigidTransformToRosTransform, py::arg("transform"));
  m.def("RosPoseToIsometry3", &drake_ros::core::RosPoseToIsometry3,
        py::arg("pose"));
  m.def("Isometry3ToRosPose", &drake_ros::core::Isometry3ToRosPose,
        py::arg("isometry"));
  m.def("RosTransformToIsometry3", &drake_ros::core::RosTransformToIsometry3,
        py::arg("transform"));
  m.def("Isometry3ToRosTransform", &drake_ros::core::Isometry3ToRosTransform,
        py::arg("isometry"));

  // Spatial Velocity
  m.def("RosTwistToVector6", &drake_ros::core::RosTwistToVector6,
        py::arg("twist"));
  m.def("Vector6ToRosTwist", &drake_ros::core::Vector6ToRosTwist,
        py::arg("vector"));
  m.def("RosTwistToSpatialVelocity",
        &drake_ros::core::RosTwistToSpatialVelocity, py::arg("twist"));
  m.def("SpatialVelocityToRosTwist",
        &drake_ros::core::SpatialVelocityToRosTwist, py::arg("velocity"));

  // Spatial Acceleration
  m.def("RosAccelToVector6", &drake_ros::core::RosAccelToVector6,
        py::arg("accel"));
  m.def("Vector6ToRosAccel", &drake_ros::core::Vector6ToRosAccel,
        py::arg("vector"));
  m.def("RosAccelToSpatialAcceleration",
        &drake_ros::core::RosAccelToSpatialAcceleration, py::arg("accel"));
  m.def("SpatialAccelerationToRosAccel",
        &drake_ros::core::SpatialAccelerationToRosAccel, py::arg("accel"));

  // Spatial Force
  m.def("RosWrenchToVector6", &drake_ros::core::RosWrenchToVector6,
        py::arg("wrench"));
  m.def("Vector6ToRosWrench", &drake_ros::core::Vector6ToRosWrench,
        py::arg("vector"));
  m.def("RosWrenchToSpatialForce", &drake_ros::core::RosWrenchToSpatialForce,
        py::arg("wrench"));
  m.def("SpatialForceToRosWrench", &drake_ros::core::SpatialForceToRosWrench,
        py::arg("force"));
}
// clang-format off
}  // namespace drake_ros_py
// clang-format on
}  // namespace drake_ros
