#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "drake/systems/sensors/rgbd_sensor.h"
#include <drake/systems/framework/diagram_builder.h>
#include <drake/systems/framework/leaf_system.h>
#include <drake_ros/core/ros_publisher_system.h>
#include <rclcpp/qos.hpp>
#include <sensor_msgs/msg/image.hpp>

using drake::systems::Context;
using drake::systems::InputPort;
using drake::systems::sensors::PixelType;
using drake::systems::sensors::RgbdSensor;

namespace drake_ros::core {

/** A system that converts drake's RGBD camera to a sensor_msgs/msg/Image
 * messages.
 */
class ImageSystem : public drake::systems::LeafSystem<double> {
 public:
  /** A constructor for the rbgd system.
   */
  ImageSystem();

  ~ImageSystem() override;

  const InputPort<double>& DeclareImageInputPort(PixelType pixel_type,
                                                 std::string port_name,
                                                 double publish_period,
                                                 double start_time);

  template <PixelType kPixelType>
  const InputPort<double>& DeclareImageInputPort(std::string port_name,
                                                 double publish_period,
                                                 double start_time);

  const InputPort<double>& DeclareDepthInputPort(PixelType pixel_type,
                                                 std::string port_name,
                                                 double publish_period,
                                                 double start_time);
  template <PixelType kPixelType>
  const InputPort<double>& DeclareDepthInputPort(std::string port_name,
                                                 double publish_period,
                                                 double start_time);

  // The per-input port data.
  struct RgbdSensorPortInfo {
    RgbdSensorPortInfo(std::string format_in, PixelType pixel_type_in)
        : format(std::move(format_in)), pixel_type(pixel_type_in) {}
    const std::string format;
    const PixelType pixel_type;
  };

  template <PixelType kPixelType>
  void PubImage(const Context<double>&, int) const;

  template <PixelType kPixelType>
  void PubDepth(const Context<double>&, int) const;

  // For each input port, this stores the corresponding image data. It is an
  // invariant that port_info_.size() == num_input_ports().
  std::vector<RgbdSensorPortInfo> port_info_;

  /** Add a ImageSystem and RosPublisherSystem to a diagram builder.
   *
   * This adds both a ImageSystem and a RosPublisherSystem that publishes
   * time to a `/image` and `depth` topics. All nodes should have their
   * `use_sim_time` parameter set to `True` so they use the published topic as
   * their source of time.
   */
  static std::pair<RosPublisherSystem*, RosPublisherSystem*> AddToBuilder(
      drake::systems::DiagramBuilder<double>* builder, DrakeRos* ros,
      const std::string& topic_name = "/image",
      const std::string& depth_topic_name = "/depth",
      const rclcpp::QoS& qos = rclcpp::SensorDataQoS(),
      const std::unordered_set<drake::systems::TriggerType>& publish_triggers =
          RosPublisherSystem::kDefaultTriggerTypes,
      double publish_period = 0.0);

 private:
  mutable sensor_msgs::msg::Image image_msgs;
  mutable sensor_msgs::msg::Image depth_msgs;
  mutable rclcpp::Time drake_time;

 protected:
  void CalcColorImage(const drake::systems::Context<double>& context,
                      sensor_msgs::msg::Image* color_value) const;

  void CalcDepthImage(const drake::systems::Context<double>& context,
                      sensor_msgs::msg::Image* depth_value) const;
};
}  // namespace drake_ros::core
