#!/usr/bin/env python3

import os
import os.path
import pathlib

import numpy

import drake_ros_core
from drake_ros_core import RosInterfaceSystem
from drake_ros_tf2 import SceneTfBroadcasterSystem
from drake_ros_tf2 import SceneTfBroadcasterParams

from drake_ros_viz import RvizVisualizer
from drake_ros_viz import RvizVisualizerParams

from pydrake.common import FindResourceOrThrow
from pydrake.geometry import DrakeVisualizer
from pydrake.math import RigidTransform
from pydrake.multibody.parsing import Parser
from pydrake.multibody.plant import AddMultibodyPlantSceneGraph
from pydrake.multibody.tree import JointIndex
from pydrake.systems.analysis import Simulator
from pydrake.systems.framework import DiagramBuilder
from pydrake.systems.framework import TriggerType
from pydrake.systems.primitives import ConstantVectorSource


if __name__ == '__main__':
    # Create a Drake diagram
    builder = DiagramBuilder()
    # Initialise the ROS infrastructure
    drake_ros_core.init()
    # Create a Drake system to interface with ROS
    sys_ros_interface = builder.AddSystem(RosInterfaceSystem('multirobot'))

    # Add a multibody plant and a scene graph to hold the robots
    plant, scene_graph = AddMultibodyPlantSceneGraph(builder, time_step=0.001)

    # Add a TF2 broadcaster to provide task frame information
    scene_tf_broadcaster = builder.AddSystem(
        SceneTfBroadcasterSystem(
            sys_ros_interface.get_ros_interface(),
            params=SceneTfBroadcasterParams(
                publish_triggers={TriggerType.kForced}
            )
        )
    )
    builder.Connect(
        scene_graph.get_query_output_port(),
        scene_tf_broadcaster.get_graph_query_port())

    # Add a system to output the visualisation markers for rviz
    scene_visualizer = builder.AddSystem(
        RvizVisualizer(
            sys_ros_interface.get_ros_interface(),
            params=RvizVisualizerParams(
                publish_triggers={TriggerType.kForced},
                publish_period=0.0
            )
        )
    )
    builder.Connect(
        scene_graph.get_query_output_port(),
        scene_visualizer.get_graph_query_port())

    # Prepare to load the robot model
    parser = Parser(plant)
    model_file_path = FindResourceOrThrow(
        'drake/manipulation/models/iiwa_description/urdf/iiwa14_polytope_collision.urdf')
    model_name = "kuka_iiwa"

    # Create a 10x10 array of manipulators
    NUM_ROWS = 10
    NUM_COLS = 10
    models = []
    for x in range(NUM_ROWS):
        models.append([])
        for y in range(NUM_COLS):
            # Load the model from the file and give it a name based on its X
            # and Y coordinates in the array
            models[x].append(parser.AddModelFromFile(
                model_file_path,
                model_name + str(x) + '_' + str(y)))

            # Weld the robot to world so it doesn't fall through floor
            base_frame = plant.GetFrameByName("base", models[x][y])
            X_WB = RigidTransform([x, y, 0])
            plant.WeldFrames(plant.world_frame(), base_frame, X_WB)

    # Finalise the multibody plant to make it ready for use
    plant.Finalize()

    # Set the control input of each robot to uncontrolled
    for x in range(NUM_ROWS):
        for y in range(NUM_COLS):
            # Get the number of degrees of freedom for the robot
            nu = plant.num_actuated_dofs(models[x][y])
            # Create a vector with the same number of zeros
            u0 = numpy.zeros(nu)
            # Create a system that emits a constant value using that vector
            constant = builder.AddSystem(ConstantVectorSource(u0))
            # Connect the constant value to the robot
            builder.Connect(
                constant.get_output_port(0),
                plant.get_actuation_input_port(models[x][y]))

    # Add a Drake visualiser instance to the diagram
    viz = DrakeVisualizer.AddToBuilder(builder, scene_graph)

    # Build the complete system from the diagram
    diagram = builder.Build()

    # Create a simulator for the system
    simulator = Simulator(diagram)
    simulator_context = simulator.get_mutable_context()
    simulator.set_target_realtime_rate(1.0)

    # Step the simulator in 0.1s intervals
    while True:
        simulator.AdvanceTo(simulator_context.get_time() + 0.1)
        # At each time step, trigger the publication of the diagram's outputs
        diagram.Publish(simulator_context)
