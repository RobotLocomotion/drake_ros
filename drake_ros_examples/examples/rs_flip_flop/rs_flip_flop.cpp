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
#include <utility>

#include <drake/systems/analysis/simulator.h>
#include <drake/systems/framework/diagram_builder.h>
#include <drake/systems/framework/leaf_system.h>
#include <drake_ros_core/drake_ros.h>
#include <drake_ros_core/ros_interface_system.h>
#include <drake_ros_core/ros_publisher_system.h>
#include <drake_ros_core/ros_subscriber_system.h>
#include <std_msgs/msg/bool.hpp>

using drake_ros_core::DrakeRos;
using drake_ros_core::RosInterfaceSystem;
using drake_ros_core::RosPublisherSystem;
using drake_ros_core::RosSubscriberSystem;

class NorGate : public drake::systems::LeafSystem<double> {
 public:
  NorGate() {
    DeclareAbstractInputPort(
        "A", *drake::AbstractValue::Make(std_msgs::msg::Bool()));
    DeclareAbstractInputPort(
        "B", *drake::AbstractValue::Make(std_msgs::msg::Bool()));

    DeclareAbstractOutputPort("Q", &NorGate::calc_output_value);
  }

  virtual ~NorGate() = default;

 private:
  void calc_output_value(const drake::systems::Context<double>& context,
                         std_msgs::msg::Bool* output) const {
    const bool a = GetInputPort("A").Eval<std_msgs::msg::Bool>(context).data;
    const bool b = GetInputPort("B").Eval<std_msgs::msg::Bool>(context).data;
    output->data = !(a || b);
  }
};

// Delay's input port by one timestep to avoid algebraic loop error
// Inspired by Simulink's Memory block
template <typename T>
class Memory : public drake::systems::LeafSystem<double> {
 public:
  explicit Memory(const T& initial_value) {
    DeclareAbstractInputPort("value", *drake::AbstractValue::Make(T()));

    // State for value
    DeclareAbstractState(drake::Value<T>(initial_value));

    // Output depends only on the previous state
    DeclareAbstractOutputPort("value", &Memory::calc_output_value,
                              {all_state_ticket()});

    DeclarePerStepEvent(drake::systems::UnrestrictedUpdateEvent<double>(
        [this](const drake::systems::Context<double>& context,
               const drake::systems::UnrestrictedUpdateEvent<double>&,
               drake::systems::State<double>* state) {
          // Copy input value to state
          drake::systems::AbstractValues& abstract_state =
              state->get_mutable_abstract_state();
          abstract_state.get_mutable_value(0).SetFrom(
              get_input_port().Eval<drake::AbstractValue>(context));
        }));
  }

  virtual ~Memory() = default;

 private:
  void calc_output_value(const drake::systems::Context<double>& context,
                         T* output) const {
    *output = context.get_abstract_state().get_value(0).get_value<T>();
  }
};

int main() {
  // NOR gate RS flip flop example
  // Input topics /S and /R are active high (true is logic 1 and false is logic
  // 0)
  // Output topics /Q and /Q_not are active low (true is logic 0 and false is
  // logic 1)

  // Input/Output table
  // S: false R: false | Q: no change  Q_not: no change
  // S: true  R: false | Q: false      Q_not: true
  // S: false R: true  | Q: true       Q_not: false
  // S: true  R: true  | Q: invalid    Q_not: invalid
  drake::systems::DiagramBuilder<double> builder;

  rclcpp::QoS qos{10};

  drake_ros_core::init();
  auto sys_ros_interface = builder.AddSystem<RosInterfaceSystem>(
      std::make_unique<DrakeRos>("rs_flip_flop_node"));
  auto sys_pub_Q =
      builder.AddSystem(RosPublisherSystem::Make<std_msgs::msg::Bool>(
          "Q", qos, sys_ros_interface->get_ros_interface()));
  auto sys_pub_Q_not =
      builder.AddSystem(RosPublisherSystem::Make<std_msgs::msg::Bool>(
          "Q_not", qos, sys_ros_interface->get_ros_interface()));
  auto sys_sub_S =
      builder.AddSystem(RosSubscriberSystem::Make<std_msgs::msg::Bool>(
          "S", qos, sys_ros_interface->get_ros_interface()));
  auto sys_sub_R =
      builder.AddSystem(RosSubscriberSystem::Make<std_msgs::msg::Bool>(
          "R", qos, sys_ros_interface->get_ros_interface()));
  auto sys_nor_gate_1 = builder.AddSystem<NorGate>();
  auto sys_nor_gate_2 = builder.AddSystem<NorGate>();
  auto sys_memory =
      builder.AddSystem<Memory<std_msgs::msg::Bool>>(std_msgs::msg::Bool());

  builder.Connect(sys_nor_gate_1->GetOutputPort("Q"),
                  sys_memory->get_input_port(0));

  builder.Connect(sys_sub_S->get_output_port(0),
                  sys_nor_gate_1->GetInputPort("A"));
  builder.Connect(sys_nor_gate_2->GetOutputPort("Q"),
                  sys_nor_gate_1->GetInputPort("B"));

  builder.Connect(sys_memory->get_output_port(0),
                  sys_nor_gate_2->GetInputPort("A"));
  builder.Connect(sys_sub_R->get_output_port(0),
                  sys_nor_gate_2->GetInputPort("B"));

  builder.Connect(sys_nor_gate_1->GetOutputPort("Q"),
                  sys_pub_Q->get_input_port(0));
  builder.Connect(sys_nor_gate_2->GetOutputPort("Q"),
                  sys_pub_Q_not->get_input_port(0));

  auto diagram = builder.Build();
  auto context = diagram->CreateDefaultContext();

  auto simulator = std::make_unique<drake::systems::Simulator<double>>(
      *diagram, std::move(context));
  simulator->set_target_realtime_rate(1.0);
  simulator->Initialize();

  auto& simulator_context = simulator->get_mutable_context();

  while (true) {
    simulator->AdvanceTo(simulator_context.get_time() + 0.1);
  }
  return 0;
}
