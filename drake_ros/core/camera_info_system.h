#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>

#include <drake/systems/framework/diagram_builder.h>
#include <drake/systems/framework/leaf_system.h>
#include <drake/systems/sensors/camera_info.h>
#include <drake_ros/core/ros_publisher_system.h>
#include <rclcpp/qos.hpp>
#include <sensor_msgs/msg/camera_info.hpp>

using drake::systems::sensors::CameraInfo;

namespace drake_ros::core {
/** A system that converts drake's camera info to a sensor_msgs/msg/CameraInfo
 * message.
 */
class CameraInfoSystem : public drake::systems::LeafSystem<double> {
 public:
  /** A constructor for the camera info system.
   */
  CameraInfoSystem();

  ~CameraInfoSystem() override;

  void SetCameraInfo(const CameraInfo& _camera_info);

  /** Add a CameraInfoSystem and RosPublisherSystem to a diagram builder.
   *
   * This adds both a CameraInfoSystem and a RosPublisherSystem that publishes
   * time to a `/image/camera_info` topic. All nodes should have their
   * `use_sim_time` parameter set to `True` so they use the published topic as
   * their source of time.
   */
  static std::tuple<CameraInfoSystem*, RosPublisherSystem*> AddToBuilder(
      drake::systems::DiagramBuilder<double>* builder, DrakeRos* ros,
      const std::string& topic_name = "/image/camera_info",
      const rclcpp::QoS& qos = rclcpp::SystemDefaultsQoS(),
      const std::unordered_set<drake::systems::TriggerType>& publish_triggers =
          RosPublisherSystem::kDefaultTriggerTypes,
      double publish_period = 0.0);

 private:
  CameraInfo camera_info;

 protected:
  void CalcCameraInfo(const drake::systems::Context<double>& context,
                      sensor_msgs::msg::CameraInfo* output_value) const;
};
}  // namespace drake_ros::core
