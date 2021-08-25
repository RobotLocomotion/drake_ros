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
#include <memory>

#include "drake_ros_tf2/scene_tf_broadcaster_system.hpp"
#include <drake/systems/framework/diagram.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "drake_ros_core/ros_interface_system.hpp"

namespace py = pybind11;

using drake::systems::Diagram;
using drake::systems::TriggerType;

using drake_ros_core::DrakeRosInterface;
using drake_ros_tf2::SceneTfBroadcasterParams;
using drake_ros_tf2::SceneTfBroadcasterSystem;

PYBIND11_MODULE(drake_ros_tf2, m) {
  m.doc() = "Python wrapper for drake_ros_tf2";

  py::module::import("drake_ros_core");

  py::module::import("pydrake.systems.framework");
  py::module::import("pydrake.multibody.plant");

  py::class_<SceneTfBroadcasterParams>(m, "SceneTfBroadcasterParams")
      .def(py::init([](py::kwargs kwargs) {
        SceneTfBroadcasterParams obj{};
        py::object pyobj = py::cast(&obj, py::return_value_policy::reference);
        for (auto& item : kwargs) {
          py::setattr(pyobj, item.first, item.second);
        }
        return obj;
      }))
      .def_readwrite("publish_triggers",
                     &SceneTfBroadcasterParams::publish_triggers)
      .def_readwrite("publish_period",
                     &SceneTfBroadcasterParams::publish_period);

  py::class_<SceneTfBroadcasterSystem, Diagram<double>>(
      m, "SceneTfBroadcasterSystem")
      .def(py::init<std::shared_ptr<DrakeRosInterface>,
                    SceneTfBroadcasterParams>(),
           py::arg("ros_interface"),
           py::arg("params") = SceneTfBroadcasterParams{})
      .def("RegisterMultibodyPlant",
           &SceneTfBroadcasterSystem::RegisterMultibodyPlant)
      .def("get_graph_query_port",
           &SceneTfBroadcasterSystem::get_graph_query_port,
           py::return_value_policy::reference_internal);
}
