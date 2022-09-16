# Copyright 2022 Open Source Robotics Foundation, Inc.
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


import pytest

import numpy as np
import random
import rclpy
import rclpy.executors
import rclpy.node
import string
import threading
import time
from visualization_msgs.msg import MarkerArray

import drake_ros_core
from drake_ros_core import RosInterfaceSystem
from drake_ros_viz import RvizVisualizer

from pydrake.examples.manipulation_station import ManipulationStation
from pydrake.systems.analysis import Simulator
from pydrake.systems.framework import DiagramBuilder
from pydrake.systems.primitives import ConstantVectorSource


class ManagedSubscription:
    def __init__(
            self,
            topic_name='/scene_markers/visual',
            required_message_count=1):
        self._context = rclpy.Context()
        self._topic_name = topic_name
        self._required_message_count = required_message_count
        self._received_messages = []
        self._spin_complete = threading.Event()

    def __enter__(self):
        self._context.init()

        # Use a randomised node name to enable parallel usage
        random.seed()
        self._node = rclpy.node.Node('managed_subscription_{}'.format(
            ''.join(random.choices(string.ascii_letters, k=10))),
            context=self._context)

        # TODO(gbiggs): When this is upstreamed, the topic type needs to be
        # parameterised.
        # TODO(gbiggs): When this is upstreamed, should it be possible to
        # subscribe to multiple topics ("ManagedSubscriptions")? Or should
        # there be a one-to-one relationship between a ManagedSubscription and
        # a subscription?
        self._subscription = self._node.create_subscription(
            MarkerArray,
            self._topic_name,
            self.callback,
            10)

        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        self._context.try_shutdown()

    def callback(self, message):
        self._received_messages.append(message)

    def spin_subscription(self, timeout=10.0):
        self._spin_complete.clear()
        self._spinning_thread = threading.Thread(
            target=lambda to: self.spinner(time.monotonic_ns(), to),
            args = ([int(timeout) * 1000000000])
            )
        self._spinning_thread.start()

    def spinner(self, start_time, timeout):
        executor = rclpy.executors.SingleThreadedExecutor(
            context=self._context)
        executor.add_node(self._node)
        while self.continue_spinning(start_time, timeout):
            executor.spin_once(timeout_sec=1)
        self._spin_complete.set()

    def continue_spinning(self, start_time, timeout):
        if len(self._received_messages) >= self._required_message_count:
            return False
        if (time.monotonic_ns() - start_time) > timeout:
            return False
        return True

    def wait_for_and_get_received_messages(self):
        self._spinning_thread.join()
        return self._received_messages

    def spin_complete(self):
        return self._spin_complete.is_set()


class DrakeTestSystem:
    def __init__(self):
        drake_ros_core.init()

        builder = DiagramBuilder()
        ros_interface_system = builder.AddSystem(
            RosInterfaceSystem('drake_ros_viz_test'))

        self.manipulation_station = builder.AddSystem(ManipulationStation())
        self.manipulation_station.SetupClutterClearingStation()
        self.manipulation_station.Finalize()

        constant_term = builder.AddSystem(ConstantVectorSource(
            np.zeros(self.manipulation_station.num_iiwa_joints())))
        builder.Connect(
            constant_term.get_output_port(),
            self.manipulation_station.GetInputPort('iiwa_position'))

        rviz_visualizer = builder.AddSystem(
            RvizVisualizer(ros_interface_system.get_ros_interface()))
        rviz_visualizer.RegisterMultibodyPlant(
            self.manipulation_station.get_multibody_plant())
        builder.Connect(
            self.manipulation_station.GetOutputPort('query_object'),
            rviz_visualizer.get_graph_query_port()
        )

        self.diagram = builder.Build()

        self.simulator = Simulator(self.diagram)
        self.simulator.Initialize()
        self.simulator.set_target_realtime_rate(1.0)
        self.context = self.simulator.get_mutable_context()
        manipulation_station_context = self.diagram.GetMutableSubsystemContext(
            self.manipulation_station,
            self.context)
        self.manipulation_station.GetInputPort('wsg_position').FixValue(
            manipulation_station_context, np.zeros(1))

    def advance(self):
        self.simulator.AdvanceTo(self.context.get_time() + 0.1)


def test_receive_visual_marker_array():
    with ManagedSubscription(required_message_count=2) as managed_subscription:
        drake_test_system = DrakeTestSystem()

        managed_subscription.spin_subscription(timeout=10)
        try:
            while not managed_subscription.spin_complete():
                drake_test_system.advance()
        except KeyboardInterrupt:
            pass
        rx_messages = managed_subscription.wait_for_and_get_received_messages()

        # Get at least two messages to confirm the markers are being updated
        assert len(rx_messages) >= 2

        # Make sure there are some markers in the array
        assert len(rx_messages[0].markers) >= 2

        # Dissect and check some important values from a marker
        marker = rx_messages[0].markers[1]
        assert marker.ns != ''
        assert marker.type == marker.MESH_RESOURCE
        assert marker.mesh_resource != ''
