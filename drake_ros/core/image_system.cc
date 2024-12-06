#include "drake_ros/core/image_system.h"

#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

#include <drake/systems/sensors/image.h>

using drake::systems::sensors::Image;
using drake::systems::sensors::ImageRgba8U;
using drake_ros::core::ImageSystem;
using drake_ros::core::RosPublisherSystem;

using drake::Value;
using drake::math::RigidTransformd;
using drake::math::RollPitchYawd;
using drake::systems::Context;
using drake::systems::EventStatus;
using drake::systems::PublishEvent;
using drake::systems::TriggerType;

ImageSystem::ImageSystem() {
  DeclareAbstractOutputPort("rgbd_color", &ImageSystem::CalcColorImage);
  DeclareAbstractOutputPort("rgbd_depth", &ImageSystem::CalcDepthImage);
}

ImageSystem::~ImageSystem() {}

void ImageSystem::CalcColorImage(const drake::systems::Context<double>& context,
                                sensor_msgs::msg::Image* color_value) const {
  color_value->header.frame_id = "CartPole/camera_optical";

  drake_time = rclcpp::Time{0, 0, RCL_ROS_TIME};
  drake_time += rclcpp::Duration::from_seconds(context.get_time());
  color_value->header.stamp = drake_time;

  color_value->height = image_msgs.height;
  color_value->width = image_msgs.width;
  color_value->step = image_msgs.step;
  color_value->encoding = image_msgs.encoding;
  color_value->data.resize(image_msgs.data.size());
  memcpy(&color_value->data[0], &image_msgs.data[0], image_msgs.data.size());
}

void ImageSystem::CalcDepthImage(
    const drake::systems::Context<double>& /*context*/,
    sensor_msgs::msg::Image* depth_value) const {
  depth_value->header.frame_id = "CartPole/camera_optical";
  depth_value->header.stamp = drake_time;

  depth_value->height = depth_msgs.height;
  depth_value->width = depth_msgs.width;
  depth_value->step = depth_msgs.step;
  depth_value->encoding = depth_msgs.encoding;
  depth_value->data.resize(depth_msgs.data.size());

  float* depth_row = reinterpret_cast<float*>(&depth_msgs.data[0]);
  float* depth_row_value = reinterpret_cast<float*>(&depth_value->data[0]);

  for (size_t x = 0; x < depth_value->data.size() / 4; ++x) {
    if (std::numeric_limits<float>::infinity() == depth_row[x]) {
      depth_row_value[x] = 0.0f;
    } else {
      depth_row_value[x] = depth_row[x];
    }
  }
}

const InputPort<double>& ImageSystem::DeclareDepthInputPort(
    PixelType pixel_type, std::string port_name, double publish_period,
    double start_time) {
  switch (pixel_type) {
    case PixelType::kRgb8U:
      break;
    case PixelType::kBgr8U:
      break;
    case PixelType::kRgba8U: {
      break;
    }
    case PixelType::kBgra8U:
      break;
    case PixelType::kDepth16U: {
      return this->template DeclareDepthInputPort<PixelType::kDepth32F>(
          std::move(port_name), publish_period, start_time);
    }
    case PixelType::kDepth32F: {
      return this->template DeclareDepthInputPort<PixelType::kDepth32F>(
          std::move(port_name), publish_period, start_time);
    }
    case PixelType::kLabel16I: {
      break;
    }
    case PixelType::kGrey8U: {
      break;
    }
    default:
      break;
  }
  throw std::logic_error(fmt::format(
      "ImageSystem::DeclareDepthInputPort does not support pixel_type={}",
      static_cast<int>(pixel_type)));
}

template <PixelType kPixelType>
const InputPort<double>& ImageSystem::DeclareDepthInputPort(
    std::string port_name, double publish_period, double start_time) {
  // Test to confirm valid pixel type.
  static_assert(
      kPixelType == PixelType::kDepth32F || kPixelType == PixelType::kDepth16U,
      "ImageWriter: the only supported pixel types are: kDepth32F "
      "and kDepth16U");

  if (publish_period <= 0) {
    throw std::logic_error("ImageSystem: publish period must be positive");
  }

  // Now configure the system for the valid port declaration.
  const auto& port =
      DeclareAbstractInputPort(port_name, Value<Image<kPixelType>>());
  // There is no DeclarePeriodicPublishEvent that accepts a lambda, so we must
  // use the advanced API to add our event.
  PublishEvent<double> event(
      TriggerType::kPeriodic,
      [port_index = port.get_index()](const System<double>& system,
                                      const Context<double>& context,
                                      const PublishEvent<double>&) {
        const auto& self = dynamic_cast<const ImageSystem&>(system);
        self.PubDepth<kPixelType>(context, port_index);
        return EventStatus::Succeeded();
      });
  DeclarePeriodicEvent<PublishEvent<double>>(publish_period, start_time, event);
  port_info_.emplace_back("depth", kPixelType);
  return port;
}

const InputPort<double>& ImageSystem::DeclareImageInputPort(
    PixelType pixel_type, std::string port_name, double publish_period,
    double start_time) {
  switch (pixel_type) {
    case PixelType::kRgb8U:
      break;
    case PixelType::kBgr8U:
      break;
    case PixelType::kRgba8U: {
      return this->template DeclareImageInputPort<PixelType::kRgba8U>(
          std::move(port_name), publish_period, start_time);
    }
    case PixelType::kBgra8U:
      break;
    case PixelType::kDepth16U: {
      break;
    }
    case PixelType::kDepth32F: {
      break;
    }
    case PixelType::kLabel16I: {
      break;
    }
    case PixelType::kGrey8U: {
      return this->template DeclareImageInputPort<PixelType::kGrey8U>(
          std::move(port_name), publish_period, start_time);
    }
    default:
      break;
  }
  throw std::logic_error(fmt::format(
      "ImageSystem::DeclareImageInputPort does not support pixel_type={}",
      static_cast<int>(pixel_type)));
}

template <PixelType kPixelType>
const InputPort<double>& ImageSystem::DeclareImageInputPort(
    std::string port_name, double publish_period, double start_time) {
  // Test to confirm valid pixel type.
  static_assert(
      kPixelType == PixelType::kRgba8U || kPixelType == PixelType::kGrey8U,
      "ImageSystem color image: the only supported pixel types are: kRgba8U "
      "and kGrey8U");

  if (publish_period <= 0) {
    throw std::logic_error("ImageSystem: publish period must be positive");
  }

  // Now configure the system for the valid port declaration.
  const auto& port =
      DeclareAbstractInputPort(port_name, Value<Image<kPixelType>>());
  // There is no DeclarePeriodicPublishEvent that accepts a lambda, so we must
  // use the advanced API to add our event.
  PublishEvent<double> event(
      TriggerType::kPeriodic,
      [port_index = port.get_index()](const System<double>& system,
                                      const Context<double>& context,
                                      const PublishEvent<double>&) {
        const auto& self = dynamic_cast<const ImageSystem&>(system);
        self.PubImage<kPixelType>(context, port_index);
        return EventStatus::Succeeded();
      });
  DeclarePeriodicEvent<PublishEvent<double>>(publish_period, start_time, event);
  port_info_.emplace_back("color", kPixelType);
  return port;
}

template <PixelType kPixelType>
void ImageSystem::PubImage(const Context<double>& context, int index) const {
  const auto& port = get_input_port(index);
  const Image<kPixelType>& image = port.Eval<Image<kPixelType>>(context);

  int channels = 4;
  std::string encoding = "rgba8";

  if (kPixelType == PixelType::kRgba8U) {
    channels = 4;
    encoding = "rgba8";
  } else if (kPixelType == PixelType::kGrey8U) {
    channels = 1;
    encoding = "mono8";
  }

  image_msgs.data.resize(image.width() * image.height() * channels);
  image_msgs.height = image.height();
  image_msgs.width = image.width();
  image_msgs.step = image.width() * channels;
  image_msgs.encoding = encoding;
  memcpy(&image_msgs.data[0], image.at(0, 0), image_msgs.data.size());
}

template <PixelType kPixelType>
void ImageSystem::PubDepth(const Context<double>& context, int index) const {
  const auto& port = get_input_port(index);
  const Image<kPixelType>& image = port.Eval<Image<kPixelType>>(context);

  std::string encoding = "32FC1";
  int octets = sizeof(float);

  if (kPixelType == PixelType::kDepth32F) {
    encoding = "32FC1";
    octets = sizeof(float);
  } else if (kPixelType == PixelType::kDepth16U) {
    encoding = "16UC1";
    octets = sizeof(uint16_t);
  }

  depth_msgs.data.resize(image.width() * image.height() * octets);
  depth_msgs.height = image.height();
  depth_msgs.width = image.width();
  depth_msgs.step = image.width() * octets;
  depth_msgs.encoding = encoding;
  memcpy(&depth_msgs.data[0], image.at(0, 0), depth_msgs.data.size());
}

template const InputPort<double>&
ImageSystem::DeclareImageInputPort<PixelType::kRgba8U>(std::string port_name,
                                                      double publish_period,
                                                      double start_time);
template const InputPort<double>&
ImageSystem::DeclareImageInputPort<PixelType::kGrey8U>(std::string port_name,
                                                      double publish_period,
                                                      double start_time);

template void ImageSystem::PubImage<PixelType::kRgba8U>(
    const Context<double>& context, int index) const;
template void ImageSystem::PubImage<PixelType::kGrey8U>(
    const Context<double>& context, int index) const;

template const InputPort<double>&
ImageSystem::DeclareDepthInputPort<PixelType::kDepth32F>(std::string port_name,
                                                        double publish_period,
                                                        double start_time);
template const InputPort<double>&
ImageSystem::DeclareDepthInputPort<PixelType::kDepth16U>(std::string port_name,
                                                        double publish_period,
                                                        double start_time);

template void ImageSystem::PubDepth<PixelType::kDepth32F>(
    const Context<double>& context, int index) const;
template void ImageSystem::PubDepth<PixelType::kDepth16U>(
    const Context<double>& context, int index) const;

std::pair<RosPublisherSystem*, RosPublisherSystem*> ImageSystem::AddToBuilder(
    drake::systems::DiagramBuilder<double>* builder, DrakeRos* ros,
    const std::string& color_topic_name, const std::string& depth_topic_name,
    const rclcpp::QoS& qos,
    const std::unordered_set<drake::systems::TriggerType>& publish_triggers,
    double publish_period) {
  auto* pub_color_system =
      builder->AddSystem(RosPublisherSystem::Make<sensor_msgs::msg::Image>(
          color_topic_name, qos, ros, publish_triggers, publish_period));

  auto* pub_depth_system =
      builder->AddSystem(RosPublisherSystem::Make<sensor_msgs::msg::Image>(
          depth_topic_name, qos, ros, publish_triggers, publish_period));

  return {pub_color_system, pub_depth_system};
}
