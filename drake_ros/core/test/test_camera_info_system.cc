
#include "drake/geometry/render_vtk/factory.h"
#include <drake/systems/analysis/simulator.h>
#include <drake/systems/framework/diagram_builder.h>
#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include "drake_ros/core/camera_info_system.h"
#include "drake_ros/core/drake_ros.h"
#include "drake_ros/core/ros_interface_system.h"

using drake_ros::core::CameraInfoSystem;
using drake_ros::core::DrakeRos;
using drake_ros::core::RosInterfaceSystem;

using drake::geometry::render::ColorRenderCamera;

TEST(Integration, camera_info_system) {
  drake_ros::core::init(0, nullptr);

  drake::systems::DiagramBuilder<double> builder;

  // Create a Drake system to interface with ROS
  auto ros_interface_system = builder.AddSystem<RosInterfaceSystem>(
      std::make_unique<DrakeRos>("test_node"));

  const ColorRenderCamera color_camera{
      {"renderer", {640, 480, M_PI_4}, {0.01, 10.0}, {}}, false};

  auto camera_info_system = CameraInfoSystem::AddToBuilder(
      &builder, ros_interface_system->get_ros_interface());

  std::get<0>(camera_info_system)
      ->SetCameraInfo(color_camera.core().intrinsics());

  auto diagram = builder.Build();
  auto context = diagram->CreateDefaultContext();

  auto simulator = std::make_unique<drake::systems::Simulator<double>>(
      *diagram, std::move(context));
  simulator->set_target_realtime_rate(1.0);
  simulator->Initialize();

  auto& simulator_context = simulator->get_mutable_context();

  // Don't need to rclcpp::init because DrakeRos uses global rclcpp::Context by
  // default
  uint msg_counter = 0;
  auto direct_ros_node = rclcpp::Node::make_shared("subscriber_node");
  auto subscription_color =
      direct_ros_node->create_subscription<sensor_msgs::msg::CameraInfo>(
          "/image/camera_info", rclcpp::SystemDefaultsQoS(),
          [&](const sensor_msgs::msg::CameraInfo::SharedPtr) {
            msg_counter++;
          });

  simulator->AdvanceTo(simulator_context.get_time() + 1.0);
  rclcpp::spin_some(direct_ros_node);
  EXPECT_GE(msg_counter, 1u);

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
