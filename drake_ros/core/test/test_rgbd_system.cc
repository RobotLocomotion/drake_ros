
#include "drake/geometry/render_vtk/factory.h"
#include <drake/multibody/plant/multibody_plant.h>
#include <drake/systems/analysis/simulator.h>
#include <drake/systems/framework/diagram_builder.h>
#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include "drake_ros/core/drake_ros.h"
#include "drake_ros/core/image_system.h"
#include "drake_ros/core/ros_interface_system.h"

using drake::multibody::AddMultibodyPlantSceneGraph;
using drake_ros::core::DrakeRos;
using drake_ros::core::ImageSystem;
using drake_ros::core::RosInterfaceSystem;

using drake::geometry::RenderEngineVtkParams;
using drake::geometry::render::ColorRenderCamera;
using drake::geometry::render::DepthRenderCamera;

using drake::math::RigidTransformd;

TEST(Integration, image_system) {
  drake_ros::core::init(0, nullptr);

  drake::systems::DiagramBuilder<double> builder;

  // Make and add the cart_pole model.
  auto [plant, scene_graph] = AddMultibodyPlantSceneGraph(&builder, 0);

  scene_graph.AddRenderer("renderer",
                          MakeRenderEngineVtk(RenderEngineVtkParams()));

  // Create a Drake system to interface with ROS
  auto ros_interface_system = builder.AddSystem<RosInterfaceSystem>(
      std::make_unique<DrakeRos>("test_node"));

  const ColorRenderCamera color_camera{
      {"renderer", {640, 480, M_PI_4}, {0.01, 10.0}, {}}, false};
  const DepthRenderCamera depth_camera{color_camera.core(), {0.01, 10.0}};
  const RigidTransformd X_WB;

  const auto world_id = scene_graph.world_frame_id();
  RgbdSensor* camera =
      builder.AddSystem<RgbdSensor>(world_id, X_WB, color_camera, depth_camera);
  builder.Connect(scene_graph.get_query_output_port(),
                  camera->query_object_input_port());

  const double image_publish_period = 1. / 30;
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

  auto [pub_color_system, pub_depth_system] = ImageSystem::AddToBuilder(
      &builder, ros_interface_system->get_ros_interface());

  builder.Connect(rgbd_publisher->GetOutputPort("rgbd_color"),
                  pub_color_system->get_input_port());

  builder.Connect(rgbd_publisher->GetOutputPort("rgbd_depth"),
                  pub_depth_system->get_input_port());

  plant.Finalize();
  auto diagram = builder.Build();
  auto context = diagram->CreateDefaultContext();

  auto simulator = std::make_unique<drake::systems::Simulator<double>>(
      *diagram, std::move(context));
  simulator->set_target_realtime_rate(1.0);
  simulator->Initialize();

  auto& simulator_context = simulator->get_mutable_context();

  // Don't need to rclcpp::init because DrakeRos uses global rclcpp::Context by
  // default
  uint color_msg_counter = 0;
  uint depth_msg_counter = 0;
  auto direct_ros_node = rclcpp::Node::make_shared("subscriber_node");
  auto subscription_color =
      direct_ros_node->create_subscription<sensor_msgs::msg::Image>(
          "/image", rclcpp::SensorDataQoS(),
          [&](const sensor_msgs::msg::Image::SharedPtr) {
            color_msg_counter++;
          });

  auto subscription_depth =
      direct_ros_node->create_subscription<sensor_msgs::msg::Image>(
          "/depth", rclcpp::SensorDataQoS(),
          [&](const sensor_msgs::msg::Image::SharedPtr) {
            depth_msg_counter++;
          });

  simulator->AdvanceTo(simulator_context.get_time() + 1.0);
  rclcpp::spin_some(direct_ros_node);
  EXPECT_GE(color_msg_counter, 1u);
  EXPECT_GE(depth_msg_counter, 1u);

  drake_ros::core::shutdown();
}

// Only available in Bazel.
#ifndef _TEST_DISABLE_RMW_ISOLATION
#include "rmw_isolation/rmw_isolation.h"

int main(int argc, char* argv[]) {
  const char* TEST_TMPDIR = std::getenv("TEST_TMPDIR");
  if (TEST_TMPDIR != nullptr) {
    std::string ros_home = std::string(TEST_TMPDIR) + "/.ros";
    setenv("ROS_HOME", ros_home.c_str(), 1);
    ros2::isolate_rmw_by_path(argv[0], TEST_TMPDIR);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
