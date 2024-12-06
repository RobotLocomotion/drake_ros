#!/usr/bin/env python3
import argparse
import numpy as np
import os

from ament_index_python.packages import get_package_share_directory

import drake_ros.core
from drake_ros.core import ClockSystem
from drake_ros.core import CameraInfoSystem
from drake_ros.core import ImageSystem
from drake_ros.core import RosInterfaceSystem

from pydrake.geometry import (ClippingRange, ColorRenderCamera, DepthRange, DepthRenderCamera,
                              MakeRenderEngineGl, RenderCameraCore, RenderEngineGlParams)
from pydrake.math import RigidTransform, RollPitchYaw
from pydrake.multibody.parsing import Parser
from pydrake.multibody.plant import AddMultibodyPlant, MultibodyPlantConfig
from pydrake.systems.analysis import Simulator
from pydrake.systems.framework import DiagramBuilder, TriggerType
from pydrake.systems.sensors import CameraInfo, PixelType, RgbdSensor
from pydrake.visualization import (
    ColorizeDepthImage,
    ColorizeLabelImage,
)

from rclpy import qos

def xyz_rpy_deg(xyz, rpy_deg):
    """Shorthand for defining a pose."""
    rpy_deg = np.asarray(rpy_deg)
    return RigidTransform(RollPitchYaw(rpy_deg * np.pi / 180), xyz)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--simulation_time',
        type=float,
        default=float('inf'),
        help='How many seconds to run the simulation')
    args = parser.parse_args()

    # Create a Drake diagram
    builder = DiagramBuilder()

    # Add a multibody plant and a scene graph to hold the robots
    plant, scene_graph = AddMultibodyPlant(
        MultibodyPlantConfig(time_step=0.001),
        builder,
    )

    renderer_name = "renderer"
    scene_graph.AddRenderer(
    renderer_name, MakeRenderEngineGl(RenderEngineGlParams()))
    parser = Parser(plant)

    package_path = get_package_share_directory("drake_ros_examples")
    parser.package_map().Add("drake_ros_examples", package_path)
    fs_path = parser.package_map().GetPath("drake_ros_examples")
    sdf_url = os.path.join(fs_path, 'rgbd', 'rgbd.sdf')

    Parser(plant, scene_graph).AddModels(sdf_url)

    # Initialise the ROS infrastructure
    drake_ros.core.init()
    # Create a Drake system to interface with ROS
    sys_ros_interface = builder.AddSystem(RosInterfaceSystem('cart_pole'))
    ClockSystem.AddToBuilder(builder, sys_ros_interface.get_ros_interface())

    camera_info_system = CameraInfoSystem.AddToBuilder(
        builder,
        sys_ros_interface.get_ros_interface(),
        topic_name='/color/camera_info',
        publish_period = 1. / 10.,
        publish_triggers={TriggerType.kPeriodic})

    depth_camera_info_system = CameraInfoSystem.AddToBuilder(
        builder,
        sys_ros_interface.get_ros_interface(),
        topic_name='/depth/camera_info',
        publish_period = 1. / 10.,
        publish_triggers={TriggerType.kPeriodic})

    intrinsics = CameraInfo(
        width=320,
        height=240,
        fov_y=np.pi/4,
    )

    core = RenderCameraCore(
        renderer_name,
        intrinsics,
        ClippingRange(0.01, 10.0),
        RigidTransform(),
    )

    color_camera = ColorRenderCamera(core, show_window=False)
    depth_camera = DepthRenderCamera(core, DepthRange(0.01, 10.0))

    camera_info_system[0].set_camera_info(intrinsics)
    depth_camera_info_system[0].set_camera_info(intrinsics)

    world_id = plant.GetBodyFrameIdOrThrow(plant.world_body().index())
    X_WB = xyz_rpy_deg([0.0, 1.0, 0.0], [90, 180, 0.0])
    sensor = RgbdSensor(
        world_id,
        X_PB=X_WB,
        color_camera=color_camera,
        depth_camera=depth_camera,
    )

    builder.AddSystem(sensor)
    builder.Connect(
        scene_graph.get_query_output_port(),
        sensor.query_object_input_port(),
    )

    colorize_depth = builder.AddSystem(ColorizeDepthImage())
    colorize_label = builder.AddSystem(ColorizeLabelImage())
    colorize_label.background_color.set([0,0,0])
    builder.Connect(sensor.GetOutputPort("depth_image_32f"),
                    colorize_depth.GetInputPort("depth_image_32f"))
    builder.Connect(sensor.GetOutputPort("label_image"),
                    colorize_label.GetInputPort("label_image"))

    rgbd_publisher = builder.AddSystem(ImageSystem())

    image_publish_period = 1. / 30.
    rgbd_port = rgbd_publisher.DeclareImageInputPort(
            PixelType.kRgba8U, "color", image_publish_period, 0.)
    builder.Connect(sensor.color_image_output_port(), rgbd_port)

    depth_port = rgbd_publisher.DeclareDepthInputPort(
            PixelType.kDepth32F, "depth", image_publish_period, 0.)
    builder.Connect(sensor.depth_image_32F_output_port(), depth_port)

    [pub_color_system, pub_depth_system] = ImageSystem.AddToBuilder(
      builder, sys_ros_interface.get_ros_interface(),
      topic_name="/color/image_raw",
      depth_topic_name='/depth/image_raw',
      publish_period = 1. / 25.,
      publish_triggers={TriggerType.kPeriodic},
      qos=qos.qos_profile_sensor_data)

    builder.Connect(rgbd_publisher.GetOutputPort("rgbd_color"),
                    pub_color_system.get_input_port())

    builder.Connect(rgbd_publisher.GetOutputPort("rgbd_depth"),
                    pub_depth_system.get_input_port())

    plant.Finalize()

    # Build the complete system from the diagram
    diagram = builder.Build()

    diagram_context = diagram.CreateDefaultContext()
    cart_pole_context = plant.GetMyMutableContextFromRoot(diagram_context)

    plant.get_actuation_input_port().FixValue(cart_pole_context, 0)

    cart_slider = plant.GetJointByName("CartSlider")
    pole_pin = plant.GetJointByName("PolePin")

    cart_slider.set_translation(context=cart_pole_context, translation=0.0)
    pole_pin.set_angle(context=cart_pole_context, angle=2.0)

    # Create a simulator for the system
    simulator = Simulator(diagram, diagram_context)
    simulator.Initialize()
    simulator_context = simulator.get_mutable_context()
    simulator.set_publish_every_time_step(False)
    simulator.set_target_realtime_rate(1.0)

    # Step the simulator.
    simulator.AdvanceTo(args.simulation_time)


if __name__ == '__main__':
    main()
