#include <memory>

#include "drake/common/drake_assert.h"
#include "drake/geometry/drake_visualizer.h"
#include "drake/geometry/render_vtk/factory.h"
#include "drake/geometry/scene_graph.h"
#include "drake/math/rigid_transform.h"
#include "drake/multibody/parsing/parser.h"
#include "drake/multibody/plant/multibody_plant.h"
#include "drake/multibody/tree/prismatic_joint.h"
#include "drake/multibody/tree/revolute_joint.h"
#include "drake/systems/analysis/simulator.h"
#include "drake/systems/framework/diagram_builder.h"
#include "drake/systems/sensors/image.h"
#include "drake/systems/sensors/pixel_types.h"
#include "drake/systems/sensors/rgbd_sensor.h"
#include "drake/visualization/visualization_config_functions.h"
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <drake_ros/core/camera_info_system.h>
#include <drake_ros/core/clock_system.h>
#include <drake_ros/core/drake_ros.h>
#include <drake_ros/core/image_system.h>
#include <drake_ros/core/ros_interface_system.h>
#include <drake_ros/tf2/scene_tf_broadcaster_system.h>
#include <drake_ros/viz/rviz_visualizer.h>
#include <gflags/gflags.h>

using drake::geometry::SceneGraph;

// "multibody" namespace is ambiguous here without "drake::".
using drake::multibody::AddMultibodyPlantSceneGraph;
using drake::multibody::MultibodyPlant;
using drake::multibody::Parser;
using drake::multibody::PrismaticJoint;
using drake::multibody::RevoluteJoint;

using drake_ros::core::CameraInfoSystem;
using drake_ros::core::ClockSystem;
using drake_ros::core::DrakeRos;
using drake_ros::core::ImageSystem;
using drake_ros::core::RosInterfaceSystem;
using drake_ros::viz::RvizVisualizer;

using drake::geometry::RenderEngineVtkParams;
using drake::geometry::render::ColorRenderCamera;
using drake::geometry::render::DepthRenderCamera;

using drake::math::RigidTransformd;
using drake::math::RollPitchYawd;
using Eigen::Vector3d;

using drake::systems::TriggerType;
using drake::systems::sensors::PixelType;
using drake::systems::sensors::RgbdSensor;

DEFINE_double(target_realtime_rate, 1.0,
              "Desired rate relative to real time.  See documentation for "
              "Simulator::set_target_realtime_rate() for details.");

DEFINE_double(simulation_time, 100.0,
              "Desired duration of the simulation in seconds.");

DEFINE_double(time_step, 0,
              "If greater than zero, the plant is modeled as a system with "
              "discrete updates and period equal to this time_step. "
              "If 0, the plant is modeled as a continuous system.");

RigidTransformd ParseCameraPose(const std::string& input_str) {
  const char delimiter = ',';
  std::vector<double> xyzrpy_numeric;

  size_t pos = 0;
  size_t next = 0;
  while ((next = input_str.find(delimiter, pos)) != std::string::npos) {
    xyzrpy_numeric.push_back(std::stod(input_str.substr(pos, next)));
    pos = next + 1;
  }
  xyzrpy_numeric.push_back(std::stod(input_str.substr(pos, next)));
  DRAKE_DEMAND(xyzrpy_numeric.size() == 6);

  const RigidTransformd X_WB(
      RollPitchYawd{xyzrpy_numeric[3], xyzrpy_numeric[4], xyzrpy_numeric[5]},
      Vector3d(xyzrpy_numeric[0], xyzrpy_numeric[1], xyzrpy_numeric[2]));

  return X_WB;
}

int do_main() {
  drake::systems::DiagramBuilder<double> builder;

  // Make and add the cart_pole model.
  auto [cart_pole, scene_graph] =
      AddMultibodyPlantSceneGraph(&builder, FLAGS_time_step);

  scene_graph.AddRenderer("renderer",
                          MakeRenderEngineVtk(RenderEngineVtkParams()));

  auto parser = Parser(&cart_pole);

  const auto package_path =
      ament_index_cpp::get_package_share_directory("drake_ros_examples");
  parser.package_map().Add("drake_ros_examples", package_path);
  std::filesystem::path fs_path{
      parser.package_map().GetPath("drake_ros_examples")};

  const std::string sdf_url = (fs_path / "rgbd" / "rgbd.sdf").string();
  Parser(&cart_pole, &scene_graph).AddModels(sdf_url);

  drake_ros::core::init();
  auto ros_interface_system = builder.AddSystem<RosInterfaceSystem>(
      std::make_unique<DrakeRos>("cart_pole"));

  const double image_publish_period = 1. / 25.0;

  auto camera_info_system = CameraInfoSystem::AddToBuilder(
      &builder, ros_interface_system->get_ros_interface(), "/color/camera_info",
      rclcpp::SystemDefaultsQoS(), {TriggerType::kPeriodic},
      image_publish_period);

  auto depth_camera_info_system = CameraInfoSystem::AddToBuilder(
      &builder, ros_interface_system->get_ros_interface(), "/depth/camera_info",
      rclcpp::SystemDefaultsQoS(), {TriggerType::kPeriodic},
      image_publish_period);

  // The size of the image is small because the performance of drake with
  // images/depth images is not good. Vtk and Gl perform similar. 640x480 is
  // generating ~4-5 fps
  const ColorRenderCamera color_camera{
      {"renderer", {320, 240, M_PI_4}, {0.01, 10.0}, {}}, false};
  const DepthRenderCamera depth_camera{color_camera.core(), {0.01, 10.0}};
  const RigidTransformd X_WB =
      ParseCameraPose("0.0, 1.0, 0.0, 1.57, 3.14, 0.0");

  std::get<0>(camera_info_system)
      ->SetCameraInfo(color_camera.core().intrinsics());
  std::get<0>(depth_camera_info_system)
      ->SetCameraInfo(depth_camera.core().intrinsics());

  const auto world_id = scene_graph.world_frame_id();
  RgbdSensor* camera =
      builder.AddSystem<RgbdSensor>(world_id, X_WB, color_camera, depth_camera);
  builder.Connect(scene_graph.get_query_output_port(),
                  camera->query_object_input_port());

  ImageSystem* rgbd_publisher{nullptr};
  rgbd_publisher = builder.template AddSystem<ImageSystem>();

  const auto& rgbd_port =
      rgbd_publisher->DeclareImageInputPort<PixelType::kRgba8U>(
          "color", image_publish_period, 0.);
  builder.Connect(camera->color_image_output_port(), rgbd_port);

  const auto& depth_port =
      rgbd_publisher->DeclareDepthInputPort<PixelType::kDepth32F>(
          "depth", image_publish_period, 0.);
  builder.Connect(camera->depth_image_32F_output_port(), depth_port);

  const double viz_dt = 1 / 60.0;
  auto rviz_visualizer = builder.AddSystem<RvizVisualizer>(
      ros_interface_system->get_ros_interface(),
      drake_ros::viz::RvizVisualizerParams{
          {TriggerType::kPeriodic}, viz_dt, true});
  builder.Connect(scene_graph.get_query_output_port(),
                  rviz_visualizer->get_graph_query_input_port());

  rviz_visualizer->RegisterMultibodyPlant(&cart_pole);

  // Add a TF2 broadcaster to provide task frame information
  auto scene_tf_broadcaster =
      builder.AddSystem<drake_ros::tf2::SceneTfBroadcasterSystem>(
          ros_interface_system->get_ros_interface(),
          drake_ros::tf2::SceneTfBroadcasterParams{
              {TriggerType::kPeriodic}, viz_dt, "/tf"});
  builder.Connect(scene_graph.get_query_output_port(),
                  scene_tf_broadcaster->get_graph_query_input_port());

  auto [pub_color_system, pub_depth_system] = ImageSystem::AddToBuilder(
      &builder, ros_interface_system->get_ros_interface(), "/color/image_raw",
      "/depth/image_raw", rclcpp::SensorDataQoS(), {TriggerType::kPeriodic},
      image_publish_period);

  builder.Connect(rgbd_publisher->GetOutputPort("rgbd_color"),
                  pub_color_system->get_input_port());

  builder.Connect(rgbd_publisher->GetOutputPort("rgbd_depth"),
                  pub_depth_system->get_input_port());

  // Now the model is complete.
  cart_pole.Finalize();

  ClockSystem::AddToBuilder(&builder, ros_interface_system->get_ros_interface(),
                            "/clock");

  auto diagram = builder.Build();

  // Create a context for this system:
  std::unique_ptr<drake::systems::Context<double>> diagram_context =
      diagram->CreateDefaultContext();
  diagram->SetDefaultContext(diagram_context.get());
  drake::systems::Context<double>& cart_pole_context =
      diagram->GetMutableSubsystemContext(cart_pole, diagram_context.get());

  // There is no input actuation in this example for the passive dynamics.
  cart_pole.get_actuation_input_port().FixValue(&cart_pole_context, 0.);

  // Get joints so that we can set initial conditions.
  const PrismaticJoint<double>& cart_slider =
      cart_pole.GetJointByName<PrismaticJoint>("CartSlider");
  const RevoluteJoint<double>& pole_pin =
      cart_pole.GetJointByName<RevoluteJoint>("PolePin");

  // Set initial state.
  cart_slider.set_translation(&cart_pole_context, 0.0);
  pole_pin.set_angle(&cart_pole_context, 2.0);

  drake::systems::Simulator<double> simulator(*diagram,
                                              std::move(diagram_context));

  simulator.set_publish_every_time_step(false);
  simulator.set_target_realtime_rate(FLAGS_target_realtime_rate);
  simulator.Initialize();

  auto& simulator_context = simulator.get_mutable_context();

  // Step the simulator in 0.1s intervals
  constexpr double kStep{0.01};
  while (simulator_context.get_time() < FLAGS_simulation_time) {
    const double next_time =
        std::min(FLAGS_simulation_time, simulator_context.get_time() + kStep);
    simulator.AdvanceTo(next_time);
  }

  return 0;
}

int main(int argc, char* argv[]) {
  gflags::SetUsageMessage(
      "A simple cart pole demo using Drake's MultibodyPlant,"
      "publishing images and camera info in ROS 2.");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return do_main();
}
