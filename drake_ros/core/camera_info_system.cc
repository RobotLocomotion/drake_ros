#include "drake_ros/core/camera_info_system.h"

#include <rclcpp/time.hpp>

using drake_ros::core::CameraInfoSystem;
using drake_ros::core::RosPublisherSystem;

CameraInfoSystem::CameraInfoSystem() : camera_info(1, 1, 1, 1, 0.5, 0.5) {
  DeclareAbstractOutputPort("CameraInfoSystem",
                            &CameraInfoSystem::CalcCameraInfo);
}

CameraInfoSystem::~CameraInfoSystem() {}

void CameraInfoSystem::CalcCameraInfo(
    const drake::systems::Context<double>& context,
    sensor_msgs::msg::CameraInfo* output_value) const {
  rclcpp::Time now{0, 0, RCL_ROS_TIME};
  now += rclcpp::Duration::from_seconds(context.get_time());
  output_value->header.stamp = now;
  output_value->header.frame_id = "CartPole/camera_optical";

  output_value->height = this->camera_info.height();
  output_value->width = this->camera_info.width();
  output_value->distortion_model = "plumb_bob";

  // distortion isn't supported. Setting this values to zero
  output_value->d.push_back(0);
  output_value->d.push_back(0);
  output_value->d.push_back(0);
  output_value->d.push_back(0);
  output_value->d.push_back(0);

  output_value->r[0] = 1;
  output_value->r[1] = 0;
  output_value->r[2] = 0;
  output_value->r[3] = 0;
  output_value->r[4] = 1;
  output_value->r[5] = 0;
  output_value->r[6] = 0;
  output_value->r[7] = 0;
  output_value->r[8] = 1;

  //      │ f_x  0    c_x │
  //  K = │ 0    f_y  c_y │
  //      │ 0    0    1   │

  //      │ f_x  0    c_x Tx│
  //  P = │ 0    f_y  c_y Ty│
  //      │ 0    0    1   0│
  output_value->k[0] = this->camera_info.focal_x();
  output_value->k[2] = this->camera_info.center_x();
  output_value->k[4] = this->camera_info.focal_y();
  output_value->k[5] = this->camera_info.center_y();
  output_value->k[8] = 1.0;

  output_value->p[0] = this->camera_info.focal_x();
  output_value->p[2] = this->camera_info.center_x();
  output_value->p[3] = 0;
  output_value->p[5] = this->camera_info.focal_y();
  output_value->p[6] = this->camera_info.center_y();
  output_value->p[10] = 1.0;
}

void CameraInfoSystem::SetCameraInfo(const CameraInfo& _camera_info) {
  this->camera_info = _camera_info;
}

std::tuple<CameraInfoSystem*, RosPublisherSystem*>
CameraInfoSystem::AddToBuilder(
    drake::systems::DiagramBuilder<double>* builder, DrakeRos* ros,
    const std::string& topic_name, const rclcpp::QoS& qos,
    const std::unordered_set<drake::systems::TriggerType>& publish_triggers,
    double publish_period) {
  auto* camera_info_system = builder->AddSystem<CameraInfoSystem>();

  auto* pub_system =
      builder->AddSystem(RosPublisherSystem::Make<sensor_msgs::msg::CameraInfo>(
          topic_name, qos, ros, publish_triggers, publish_period));

  builder->Connect(camera_info_system->get_output_port(),
                   pub_system->get_input_port());

  return {camera_info_system, pub_system};
}
