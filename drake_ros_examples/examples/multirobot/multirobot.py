#!/usr/bin/env python3

import os
import os.path
import pathlib

import numpy

from ament_index_python.packages import get_package_share_directory

from drake_ros_core import RosInterfaceSystem
from drake_ros_tf2 import SceneTfBroadcasterSystem
from drake_ros_tf2 import SceneTfBroadcasterParams

from drake_ros_viz import RvizVisualizer
from drake_ros_viz import RvizVisualizerParams

from pydrake.geometry import DrakeVisualizer
from pydrake.math import RigidTransform
from pydrake.multibody.parsing import Parser
from pydrake.multibody.plant import AddMultibodyPlantSceneGraph
from pydrake.multibody.tree import JointIndex
from pydrake.systems.analysis import Simulator
from pydrake.systems.framework import DiagramBuilder
from pydrake.systems.framework import TriggerType
from pydrake.systems.primitives import ConstantVectorSource

# RuntimeError: Actuation input port for model instance iiwa must be connected.
def no_control(plant, builder, model):
    nu = plant.num_actuated_dofs(model)
    u0 = numpy.zeros(nu)
    constant = builder.AddSystem(ConstantVectorSource(u0))
    builder.Connect(
        constant.get_output_port(0),
        plant.get_actuation_input_port(model))


NUM_ROWS = 10
NUM_COLS = 10
if __name__ == '__main__':
    builder = DiagramBuilder()
    sys_ros_interface = builder.AddSystem(RosInterfaceSystem())

    plant, scene_graph = AddMultibodyPlantSceneGraph(builder, time_step=0.001)

    scene_tf_broadcaster = builder.AddSystem(
        SceneTfBroadcasterSystem(
            sys_ros_interface.get_ros_interface(),
            params=SceneTfBroadcasterParams(
                publish_triggers={TriggerType.kForced}
            )
        )
    )

    scene_visualizer = builder.AddSystem(
        RvizVisualizer(
            sys_ros_interface.get_ros_interface(),
            params=RvizVisualizerParams(
                publish_triggers={TriggerType.kPeriodic}
            )
        )
    )

    builder.Connect(
        scene_graph.get_query_output_port(),
        scene_tf_broadcaster.get_graph_query_port())

    builder.Connect(
        scene_graph.get_query_output_port(),
        scene_visualizer.get_graph_query_port())

    parser = Parser(plant)
    parser.package_map().PopulateFromEnvironment('AMENT_PREFIX_PATH')
    package_share_directory = get_package_share_directory('drake_ros_examples')
    model_file_path = os.path.join(
        package_share_directory,
        'iiwa_description/urdf/iiwa14_polytope_collision.urdf')

    model_name = "kuka_iiwa"

    models = []
    for x in range(NUM_ROWS):
        models.append([])
        for y in range(NUM_COLS):

            models[x].append(parser.AddModelFromFile(model_file_path, model_name + str(x) + '_' + str(y)))

            # Weld to world so it doesn't fall through floor :D
            base_frame = plant.GetFrameByName("base", models[x][y])
            X_WB = RigidTransform([x, y, 0])
            plant.WeldFrames(plant.world_frame(), base_frame, X_WB)

    plant.Finalize()

    for x in range(NUM_ROWS):
        for y in range(NUM_COLS):
            # Must happen after Finalize
            # RuntimeError: Pre-finalize calls to 'num_actuated_dofs()' are not allowed; you must call Finalize() first.
            no_control(plant, builder, models[x][y])

    viz = DrakeVisualizer.AddToBuilder(builder, scene_graph)

    diagram = builder.Build()
    simulator = Simulator(diagram)
    simulator_context = simulator.get_mutable_context()
    simulator.set_target_realtime_rate(1.0)

    while True:
        simulator.AdvanceTo(simulator_context.get_time() + 0.1)
        diagram.Publish(simulator_context)
