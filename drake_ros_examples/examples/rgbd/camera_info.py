#!/usr/bin/env python3

import numpy as np

import drake_ros.core
from drake_ros.core import ClockSystem
from drake_ros.core import CameraInfoSystem
from drake_ros.core import RosInterfaceSystem

from pydrake.common import FindResourceOrThrow
from pydrake.multibody.parsing import Parser
from pydrake.systems.analysis import Simulator
from pydrake.systems.framework import DiagramBuilder
from pydrake.systems.sensors import CameraInfo


def main():
    # Create a Drake diagram
    builder = DiagramBuilder()
    # Initialise the ROS infrastructure
    drake_ros.core.init()
    # Create a Drake system to interface with ROS
    sys_ros_interface = builder.AddSystem(RosInterfaceSystem('camera_info'))
    ClockSystem.AddToBuilder(builder, sys_ros_interface.get_ros_interface())
    camera_info_system = CameraInfoSystem.AddToBuilder(builder, sys_ros_interface.get_ros_interface())

    intrinsics = CameraInfo(
        width=640,
        height=480,
        fov_y=np.pi/4,
    )

    camera_info_system[0].set_camera_info(intrinsics)

    # Build the complete system from the diagram
    diagram = builder.Build()

    # Create a simulator for the system
    simulator = Simulator(diagram)
    simulator.Initialize()
    simulator_context = simulator.get_mutable_context()
    simulator.set_target_realtime_rate(1.0)

    # Step the simulator in 0.1s intervals
    step = 0.1
    while simulator_context.get_time() < float('inf'):
        next_time = min(
            simulator_context.get_time() + step, float('inf'),
        )
        simulator.AdvanceTo(next_time)


if __name__ == '__main__':
    main()
