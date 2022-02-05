# Copyright 2020 Open Source Robotics Foundation, Inc.
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

from drake_ros_core import RosInterfaceSystem
from drake_ros_core import RosPublisherSystem
from drake_ros_core import RosSubscriberSystem

from pydrake.systems.analysis import Simulator
from pydrake.systems.framework import DiagramBuilder
from pydrake.systems.framework import UnrestrictedUpdateEvent
from pydrake.common.value import AbstractValue
from pydrake.systems.framework import LeafSystem

from std_msgs.msg import Bool

from rclpy.qos import QoSProfile


class NorGate(LeafSystem):

    def __init__(self):
        super().__init__()
        self._a = self.DeclareAbstractInputPort("A", AbstractValue.Make(Bool()))
        self._b = self.DeclareAbstractInputPort("B", AbstractValue.Make(Bool()))

        self.DeclareAbstractOutputPort(
            'Q',
            lambda: AbstractValue.Make(Bool()),
            self._calc_output_value)

    def _calc_output_value(self, context, data):
        a = self._a.Eval(context)
        b = self._b.Eval(context)
        data.get_mutable_value().data = not (a.data or b.data)


class Memory(LeafSystem):
    """Delay input port by one time step to avoid algebraic loop error."""

    def __init__(self, initial_value):
        super().__init__()

        self._input = self.DeclareAbstractInputPort("A", AbstractValue.Make(initial_value))

        self.DeclareAbstractState(AbstractValue.Make(initial_value))

        self.DeclareAbstractOutputPort(
            'Q',
            lambda: AbstractValue.Make(initial_value),
            self._calc_output_value,
            {self.all_state_ticket()})

        self.DeclarePerStepEvent(UnrestrictedUpdateEvent(self._move_input_to_state))

    def _move_input_to_state(self, context, event, state):
        state.get_mutable_abstract_state().get_mutable_value(0).SetFrom(
            AbstractValue.Make(self._input.Eval(context)))

    def _calc_output_value(self, context, output):
        output.SetFrom(context.get_abstract_state().get_value(0))


def main():
    # NOR gate RS flip flop example
    # Input topics /S and /R are active high (true is logic 1 and false is logic 0)
    # Output topics  /Q and /Q_not are active low (true is logic 0 and false is logic 1)

    # Input/Output table
    # S: false R: false | Q: no change  Q_not: no change
    # S: true  R: false | Q: false      Q_not: true
    # S: false R: true  | Q: true       Q_not: false
    # S: true  R: true  | Q: invalid    Q_not: invalid
    builder = DiagramBuilder()

    sys_ros_interface = builder.AddSystem(RosInterfaceSystem())

    qos = QoSProfile(depth=10)

    sys_pub_Q = builder.AddSystem(
        RosPublisherSystem(Bool, "Q", qos, sys_ros_interface.get_ros_interface()))
    sys_pub_Q_not = builder.AddSystem(
        RosPublisherSystem(Bool, "Q_not", qos, sys_ros_interface.get_ros_interface()))

    sys_sub_S = builder.AddSystem(
        RosSubscriberSystem(Bool, "S", qos, sys_ros_interface.get_ros_interface()))
    sys_sub_R = builder.AddSystem(
        RosSubscriberSystem(Bool, "R", qos, sys_ros_interface.get_ros_interface()))

    sys_nor_gate_1 = builder.AddSystem(NorGate())
    sys_nor_gate_2 = builder.AddSystem(NorGate())

    sys_memory = builder.AddSystem(Memory(Bool()))

    builder.Connect(
        sys_nor_gate_1.GetOutputPort('Q'),
        sys_memory.get_input_port(0)
    )

    builder.Connect(
        sys_sub_S.get_output_port(0),
        sys_nor_gate_1.GetInputPort('A'),
    )
    builder.Connect(
        sys_nor_gate_2.GetOutputPort('Q'),
        sys_nor_gate_1.GetInputPort('B'),
    )

    builder.Connect(
        sys_memory.get_output_port(0),
        sys_nor_gate_2.GetInputPort('A'),
    )
    builder.Connect(
        sys_sub_R.get_output_port(0),
        sys_nor_gate_2.GetInputPort('B'),
    )

    builder.Connect(
        sys_nor_gate_1.GetOutputPort('Q'),
        sys_pub_Q.get_input_port(0)
    )
    builder.Connect(
        sys_nor_gate_2.GetOutputPort('Q'),
        sys_pub_Q_not.get_input_port(0)
    )

    diagram = builder.Build()
    simulator = Simulator(diagram)
    simulator_context = simulator.get_mutable_context()
    simulator.set_target_realtime_rate(1.0)

    while True:
        simulator.AdvanceTo(simulator_context.get_time() + 0.1)
