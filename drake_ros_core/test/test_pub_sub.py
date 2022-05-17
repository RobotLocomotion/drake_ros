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

import os

from pydrake.systems.analysis import Simulator
from pydrake.systems.framework import DiagramBuilder
from pydrake.systems.framework import TriggerType

import rclpy
from rclpy.qos import DurabilityPolicy
from rclpy.qos import HistoryPolicy
from rclpy.qos import QoSProfile
from rclpy.qos import ReliabilityPolicy

from test_msgs.msg import BasicTypes

import drake_ros_core
from drake_ros_core import RosInterfaceSystem
from drake_ros_core import RosPublisherSystem
from drake_ros_core import RosSubscriberSystem


def test_nominal_case():
    if 'TEST_TMPDIR' in os.environ:
        from rmw_isolation import isolate_rmw_by_path
        isolate_rmw_by_path(os.environ['TEST_TMPDIR'])

    drake_ros_core.init()

    builder = DiagramBuilder()

    system_ros = builder.AddSystem(
        RosInterfaceSystem('pub_to_sub_py'))

    publish_period = 1.0

    qos = QoSProfile(
        depth=10,
        history=HistoryPolicy.KEEP_LAST,
        reliability=ReliabilityPolicy.RELIABLE)

    system_pub_out = builder.AddSystem(RosPublisherSystem.Make(
        BasicTypes, 'out_py', qos, system_ros.get_ros_interface(),
        {TriggerType.kPeriodic}, publish_period))

    system_sub_in = builder.AddSystem(RosSubscriberSystem.Make(
        BasicTypes, 'in_py', qos, system_ros.get_ros_interface()))

    builder.Connect(
        system_sub_in.get_output_port(0),
        system_pub_out.get_input_port(0))
    diagram = builder.Build()
    simulator = Simulator(diagram)
    simulator.set_target_realtime_rate(1.0)
    simulator.Initialize()

    simulator_context = simulator.get_mutable_context()

    rclpy.init()
    direct_ros_node = rclpy.create_node('sub_to_pub_py')

    # Create publisher talking to subscriber system.
    direct_pub_in = direct_ros_node.create_publisher(BasicTypes, 'in_py', qos)

    # Create subscription listening to publisher system
    rx_msgs_direct_sub_out = []

    def rx_callback_direct_sub_out(msg):
        rx_msgs_direct_sub_out.append(msg)
    direct_sub_out = direct_ros_node.create_subscription(
        BasicTypes, 'out_py', rx_callback_direct_sub_out, qos)

    # Wait for the subscriber to connect
    for i in range(1, 10):
        if direct_pub_in.get_subscription_count() > 0:
            break
        rclpy.spin_once(direct_ros_node, timeout_sec=0.1)
    else:
        assert False, 'Timeout waiting for publisher and subscriber to connect'

    pub_sub_rounds = 5
    for i in range(1, pub_sub_rounds + 1):
        rx_msgs_count_before_pubsub = len(rx_msgs_direct_sub_out)
        # Publish a message to the drake ros subscriber system.
        message = BasicTypes()
        message.uint64_value = i
        direct_pub_in.publish(message)
        # Step forward to allow the message to be dispatched to the drake ros
        # subscriber system. The drake ros publisher system should not publish
        # just yet.
        rclpy.spin_once(direct_ros_node, timeout_sec=0.)
        simulator.AdvanceTo(simulator_context.get_time() + publish_period / 2.)
        assert len(rx_msgs_direct_sub_out) == rx_msgs_count_before_pubsub
        # Step forward until it is about time the drake ros publisher
        # publishes. Allow the message to be dispatched to the direct
        # subscription.
        simulator.AdvanceTo(simulator_context.get_time() + publish_period / 2.)
        rclpy.spin_once(direct_ros_node, timeout_sec=0.)
        rx_msgs_count_after_pubsub = rx_msgs_count_before_pubsub + 1
        assert len(rx_msgs_direct_sub_out) == rx_msgs_count_after_pubsub
        assert rx_msgs_direct_sub_out[-1].uint64_value == i

    drake_ros_core.shutdown()
