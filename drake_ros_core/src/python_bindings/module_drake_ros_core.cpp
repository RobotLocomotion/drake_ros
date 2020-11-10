// Copyright 2020 Open Source Robotics Foundation, Inc.
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
#include <drake/systems/framework/leaf_system.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>
#include <unordered_set>

#include "drake_ros_core/drake_ros.hpp"
#include "drake_ros_core/ros_interface_system.hpp"
#include "drake_ros_core/ros_publisher_system.hpp"
#include "drake_ros_core/ros_subscriber_system.hpp"

#include "py_serializer.hpp"
#include "qos_type_caster.hpp"

namespace py = pybind11;

using drake::systems::Diagram;
using drake::systems::LeafSystem;
using drake::systems::TriggerType;

using drake_ros_core::DrakeRos;
using drake_ros_core::DrakeRosInterface;
using drake_ros_core::PySerializer;
using drake_ros_core::RosInterfaceSystem;
using drake_ros_core::RosPublisherSystem;
using drake_ros_core::RosSubscriberSystem;
using drake_ros_core::SerializerInterface;


PYBIND11_MODULE(drake_ros_core, m) {
  m.doc() = "Python wrapper for drake_ros_core";

  py::module::import("pydrake.systems.framework");
  py::module::import("pydrake.multibody.plant");

  // Use std::shared_ptr holder so pybind11 doesn't try to delete interfaces returned from
  // get_ros_interface
  py::class_<DrakeRosInterface, std::shared_ptr<DrakeRosInterface>>(m, "DrakeRosInterface");

  py::class_<RosInterfaceSystem, LeafSystem<double>>(m, "RosInterfaceSystem")
  .def(
    py::init(
      []() {return std::make_unique<RosInterfaceSystem>(std::make_unique<DrakeRos>());}))
  .def("get_ros_interface", &RosInterfaceSystem::get_ros_interface);

  py::class_<RosPublisherSystem, LeafSystem<double>>(m, "RosPublisherSystem")
  .def(
    py::init(
      [](
        pybind11::object type,
        const char * topic_name,
        drake_ros_core::QoS qos,
        std::shared_ptr<DrakeRosInterface> ros_interface)
      {
        std::unique_ptr<SerializerInterface> serializer = std::make_unique<PySerializer>(type);
        return std::make_unique<RosPublisherSystem>(
          serializer,
          topic_name,
          qos,
          ros_interface,
          std::unordered_set<TriggerType>{TriggerType::kPerStep, TriggerType::kForced},
          0.0);
      }))
  .def(
    py::init(
      [](
        pybind11::object type,
        const char * topic_name,
        drake_ros_core::QoS qos,
        std::shared_ptr<DrakeRosInterface> ros_interface,
        std::unordered_set<TriggerType> publish_triggers,
        double publish_period)
      {
        std::unique_ptr<SerializerInterface> serializer = std::make_unique<PySerializer>(type);
        return std::make_unique<RosPublisherSystem>(
          serializer,
          topic_name,
          qos,
          ros_interface,
          publish_triggers,
          publish_period);
      }));

  py::class_<RosSubscriberSystem, LeafSystem<double>>(m, "RosSubscriberSystem")
  .def(
    py::init(
      [](
        pybind11::object type,
        const char * topic_name,
        drake_ros_core::QoS qos,
        std::shared_ptr<DrakeRosInterface> ros_interface)
      {
        std::unique_ptr<SerializerInterface> serializer = std::make_unique<PySerializer>(type);
        return std::make_unique<RosSubscriberSystem>(
          serializer,
          topic_name,
          qos,
          ros_interface);
      }));
}
