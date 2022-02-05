#!/usr/bin/env python3
# Copyright 2021 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import numpy as np

from drake_ros_systems import RosInterfaceSystem
from drake_ros_systems import RvizVisualizer

from pydrake.examples.manipulation_station import ManipulationStation
from pydrake.systems.analysis import Simulator
from pydrake.systems.framework import DiagramBuilder
from pydrake.systems.primitives import Adder
from pydrake.systems.primitives import ConstantVectorSource
from pydrake.systems.primitives import Sine


def main():
    builder = DiagramBuilder()

    ros_interface_system = builder.AddSystem(RosInterfaceSystem())

    manipulation_station = builder.AddSystem(ManipulationStation())
    manipulation_station.SetupClutterClearingStation()
    manipulation_station.Finalize()

    # Make the base joint swing sinusoidally.
    constant_term = builder.AddSystem(ConstantVectorSource(
        np.zeros(manipulation_station.num_iiwa_joints())))

    amplitudes = np.zeros(manipulation_station.num_iiwa_joints())
    amplitudes[0] = np.pi / 4.  # == 45 degrees
    frequencies = np.ones(manipulation_station.num_iiwa_joints())
    phases = np.zeros(manipulation_station.num_iiwa_joints())
    variable_term = builder.AddSystem(Sine(amplitudes, frequencies, phases))

    joint_trajectory_generator = builder.AddSystem(
        Adder(2, manipulation_station.num_iiwa_joints()))

    builder.Connect(constant_term.get_output_port(),
                    joint_trajectory_generator.get_input_port(0))
    builder.Connect(variable_term.get_output_port(0),
                    joint_trajectory_generator.get_input_port(1))
    builder.Connect(joint_trajectory_generator.get_output_port(),
                    manipulation_station.GetInputPort('iiwa_position'))

    rviz_visualizer = builder.AddSystem(
        RvizVisualizer(ros_interface_system.get_ros_interface()))

    rviz_visualizer.RegisterMultibodyPlant(
        manipulation_station.get_multibody_plant())

    builder.Connect(
        manipulation_station.GetOutputPort('query_object'),
        rviz_visualizer.get_graph_query_port()
    )

    diagram = builder.Build()

    simulator = Simulator(diagram)
    simulator.Initialize()
    simulator.set_target_realtime_rate(1.0)
    context = simulator.get_mutable_context()

    manipulation_station_context = \
        diagram.GetMutableSubsystemContext(manipulation_station, context)
    constant_term_context = \
        diagram.GetMutableSubsystemContext(constant_term, context)

    # Fix gripper joints' position.
    manipulation_station.GetInputPort('wsg_position').FixValue(
        manipulation_station_context, np.zeros(1))

    # Use default positions for every joint but the base joint.
    constants = constant_term.get_mutable_source_value(constant_term_context)
    constants.set_value(
        manipulation_station.GetIiwaPosition(manipulation_station_context))
    constants.get_mutable_value()[0] = -np.pi / 4.

    try:
        while True:
            simulator.AdvanceTo(context.get_time() + 0.1)
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
