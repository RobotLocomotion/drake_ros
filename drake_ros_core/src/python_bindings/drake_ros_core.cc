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
#include <unordered_set>

#include "py_serializer_interface_pybind.h"  // NOLINT(build/include)
#include "qos_type_caster_pybind.h"          // NOLINT(build/include)
#include <drake/systems/framework/leaf_system.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "drake_ros_core/drake_ros.h"
#include "drake_ros_core/ros_interface_system.h"
#include "drake_ros_core/ros_publisher_system.h"
#include "drake_ros_core/ros_subscriber_system.h"

namespace py = pybind11;

using drake::systems::LeafSystem;
using drake::systems::TriggerType;

using drake_ros_core::DrakeRos;
using drake_ros_core::PySerializerInterface;
using drake_ros_core::RosInterfaceSystem;
using drake_ros_core::RosPublisherSystem;
using drake_ros_core::RosSubscriberSystem;
using drake_ros_core::SerializerInterface;

PYBIND11_MODULE(_drake_ros_core, m) {
  m.doc() = "Python bindings for drake_ros_core";
  // Force module name in docstrings to match
  // that of the outer module.
  m.attr("__name__") = "drake_ros_core";

  py::module::import("pydrake.systems.framework");
  py::module::import("pydrake.multibody.plant");

  // TODD(hidmic): populate Python docstrings with
  // C++ docstrings. Consider using mkdoc to keep
  // them in sync, like pydrake does.
  py::class_<DrakeRos>(m, "DrakeRos");

  m.def(
      "init",
      [](std::vector<std::string> args) {
        std::vector<const char*> raw_args;
        raw_args.reserve(args.size());
        for (auto& arg : args) {
          raw_args.push_back(arg.c_str());
        }
        drake_ros_core::init(raw_args.size(), raw_args.data());
      },
      py::arg("args") = std::vector<std::string>{});
  m.def("shutdown", &drake_ros_core::shutdown);

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
                       const char* topic_name, drake_ros_core::QoS qos,
                       DrakeRos* ros_interface,
                       std::unordered_set<TriggerType> publish_triggers,
                       double publish_period) {
        return std::make_unique<RosPublisherSystem>(
            std::move(serializer), topic_name, qos, ros_interface,
            publish_triggers, publish_period);
      }));

  py::class_<RosSubscriberSystem, LeafSystem<double>>(m, "RosSubscriberSystem")
      .def(py::init([](std::unique_ptr<SerializerInterface> serializer,
                       const char* topic_name, drake_ros_core::QoS qos,
                       DrakeRos* ros_interface) {
        return std::make_unique<RosSubscriberSystem>(
            std::move(serializer), topic_name, qos, ros_interface);
      }));
}
