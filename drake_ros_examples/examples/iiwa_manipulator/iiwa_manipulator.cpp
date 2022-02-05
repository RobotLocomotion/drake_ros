// Copyright 2021https://www.youtube.com/watch?v=SUQnduNzsw8 Open Source Robotics Foundation, Inc.
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

#include <drake/common/eigen_types.h>
#include <drake/systems/analysis/simulator.h>
#include <drake/systems/framework/diagram_builder.h>
#include <drake/systems/primitives/adder.h>
#include <drake/systems/primitives/constant_vector_source.h>
#include <drake/systems/primitives/sine.h>


#include <drake/examples/manipulation_station/manipulation_station.h>

#include <drake_ros_core/drake_ros.hpp>
#include <drake_ros_core/ros_interface_system.hpp>
#include <drake_ros_viz/rviz_visualizer.hpp>

#include <cmath>
#include <memory>
#include <utility>

using drake_ros_core::DrakeRos;
using drake_ros_core::RosInterfaceSystem;
using drake_ros_viz::RvizVisualizer;

using drake::systems::Adder;
using drake::systems::ConstantVectorSource;
using drake::systems::Sine;
using drake::systems::Simulator;
using drake::examples::manipulation_station::ManipulationStation;


int main()
{
  drake::systems::DiagramBuilder<double> builder;

  auto ros_interface_system =
    builder.AddSystem<RosInterfaceSystem>(std::make_unique<DrakeRos>());

  auto manipulation_station = builder.AddSystem<ManipulationStation>();
  manipulation_station->SetupClutterClearingStation();
  manipulation_station->Finalize();

  // Make the base joint swing sinusoidally.
  auto constant_term = builder.AddSystem<ConstantVectorSource>(
    drake::VectorX<double>::Zero(manipulation_station->num_iiwa_joints()));

  drake::VectorX<double> amplitudes =
    drake::VectorX<double>::Zero(manipulation_station->num_iiwa_joints());
  amplitudes[0] = M_PI / 4.;  // == 45 degrees
  const drake::VectorX<double> frequencies =
    drake::VectorX<double>::Constant(manipulation_station->num_iiwa_joints(), 1.);  // Hz
  const drake::VectorX<double> phases =
    drake::VectorX<double>::Zero(manipulation_station->num_iiwa_joints());
  auto variable_term = builder.AddSystem<Sine>(amplitudes, frequencies, phases);

  auto joint_trajectory_generator =
    builder.AddSystem<Adder>(2, manipulation_station->num_iiwa_joints());

  builder.Connect(
    constant_term->get_output_port(),
    joint_trajectory_generator->get_input_port(0));
  builder.Connect(
    variable_term->get_output_port(0),
    joint_trajectory_generator->get_input_port(1));
  builder.Connect(
    joint_trajectory_generator->get_output_port(),
    manipulation_station->GetInputPort("iiwa_position"));

  auto rviz_visualizer = builder.AddSystem<RvizVisualizer>(
    ros_interface_system->get_ros_interface());

  rviz_visualizer->RegisterMultibodyPlant(
    &manipulation_station->get_multibody_plant());

  builder.Connect(
    manipulation_station->GetOutputPort("query_object"),
    rviz_visualizer->get_graph_query_port());

  auto diagram = builder.Build();
  auto context = diagram->CreateDefaultContext();

  auto simulator = std::make_unique<Simulator<double>>(*diagram, std::move(context));
  simulator->set_target_realtime_rate(1.0);
  simulator->Initialize();

  auto & simulator_context = simulator->get_mutable_context();

  auto & manipulation_station_context =
    diagram->GetMutableSubsystemContext(*manipulation_station, &simulator_context);

  auto & constant_term_context =
    diagram->GetMutableSubsystemContext(*constant_term, &simulator_context);

  // Fix gripper joints' position.
  manipulation_station->GetInputPort("wsg_position")
  .FixValue(&manipulation_station_context, 0.);

  // Use default positions for every joint but the base joint.
  drake::systems::BasicVector<double> & constants =
    constant_term->get_mutable_source_value(&constant_term_context);
  constants.set_value(
    manipulation_station->GetIiwaPosition(manipulation_station_context));
  constants.get_mutable_value()[0] = -M_PI / 4.;

  while (true) {
    simulator->AdvanceTo(simulator_context.get_time() + 0.1);
  }
  return 0;
}
