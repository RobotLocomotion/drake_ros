// Copyright 2022 Open Source Robotics Foundation, Inc.
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

#include <ament_index_cpp/get_package_share_directory.hpp>

#include <drake/geometry/drake_visualizer.h>
#include <drake/multibody/plant/multibody_plant.h>
#include <drake/multibody/parsing/parser.h>
#include <drake/systems/analysis/simulator.h>
#include <drake/systems/framework/diagram_builder.h>
#include <drake/systems/primitives/constant_vector_source.h>

#include <drake_ros_core/drake_ros.h>
#include <drake_ros_core/ros_interface_system.h>
#include <drake_ros_tf2/scene_tf_broadcaster_system.h>
#include <drake_ros_viz/rviz_visualizer.h>

#include <filesystem>
#include <memory>

using drake_ros_core::DrakeRos;
using drake_ros_core::RosInterfaceSystem;

using drake::systems::ConstantVectorSource;
using drake::systems::Simulator;


int main()
{
  drake::systems::DiagramBuilder<double> builder;

  drake_ros_core::init();
  auto ros_interface_system =
    builder.AddSystem<RosInterfaceSystem>(std::make_unique<DrakeRos>("multirobot_node"));

  auto [plant, scene_graph] = drake::multibody::AddMultibodyPlantSceneGraph(&builder, 0.001);

  auto scene_tf_broadcaster = builder.AddSystem<drake_ros_tf2::SceneTfBroadcasterSystem>(
    ros_interface_system->get_ros_interface(),
    drake_ros_tf2::SceneTfBroadcasterParams{
      {drake::systems::TriggerType::kForced}, 0., "/tf"});

  auto scene_visualizer = builder.AddSystem<drake_ros_viz::RvizVisualizer>(
    ros_interface_system->get_ros_interface(),
    drake_ros_viz::RvizVisualizerParams{
      {drake::systems::TriggerType::kPeriodic}, 0.05, true});

  builder.Connect(scene_graph.get_query_output_port(),
    scene_tf_broadcaster->get_graph_query_port());
  builder.Connect(scene_graph.get_query_output_port(),
    scene_visualizer->get_graph_query_port());

  auto parser = drake::multibody::Parser(&plant);
  parser.package_map().PopulateFromEnvironment("AMENT_PREFIX_PATH");
  std::filesystem::path model_file_path = 
    ament_index_cpp::get_package_share_directory("drake_ros_examples");
  model_file_path /= "iiwa_description/urdf/iiwa14_polytope_collision.urdf";

  std::string model_name = "kuka_iiwa";

  size_t kNumRows = 10;
  size_t kNumCols = 10;
  std::vector<std::vector<drake::multibody::ModelInstanceIndex>> models;
  for (uint8_t xx = 0; xx < kNumRows; ++xx) {
    std::vector<drake::multibody::ModelInstanceIndex> models_xx;
    for (uint8_t yy = 0; yy < kNumCols; ++yy) {
      std::stringstream model_instance_name;
      model_instance_name << model_name << xx << '_' << yy;
      auto model_instance = parser.AddModelFromFile(model_file_path, model_instance_name.str());

      // Weld the model to the world so it doesn't fall through the floor
      auto & base_frame = plant.GetFrameByName("base", model_instance);
      auto X_WB = drake::math::RigidTransform(drake::Vector3<double>{
        static_cast<double>(xx),
        static_cast<double>(yy),
        0.});
      plant.WeldFrames(plant.world_frame(), base_frame, X_WB);

      models_xx.push_back(model_instance);
    }
    models.push_back(models_xx);
  }

  plant.Finalize();

  for (size_t xx = 0; xx < kNumRows; ++xx) {
    for (size_t yy = 0; yy < kNumCols; ++yy) {
      auto num_dofs = plant.num_actuated_dofs(models[xx][yy]);
      auto u0 = drake::VectorX<double>::Zero(num_dofs);
      auto constant = builder.AddSystem<ConstantVectorSource>(u0);
      builder.Connect(
        constant->get_output_port(),
        plant.get_actuation_input_port(models[xx][yy]));
    }
  }

  drake::geometry::DrakeVisualizer<double>::AddToBuilder(&builder, scene_graph);

  auto diagram = builder.Build();
  auto simulator = std::make_unique<Simulator<double>>(*diagram);
  auto & simulator_context = simulator->get_mutable_context();
  simulator->set_target_realtime_rate(1.0);

  while (true) {
    simulator->AdvanceTo(simulator_context.get_time() + 0.1);
    diagram->Publish(simulator_context);
  }

  return 0;
}
