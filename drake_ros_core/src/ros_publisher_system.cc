// Copyright 2021 Open Source Robotics Foundation, Inc.
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

#include "drake_ros_core/ros_publisher_system.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "publisher.h"  // NOLINT(build/include)

#include "drake_ros_core/serializer_interface.h"

namespace drake_ros_core {
struct RosPublisherSystem::Impl {
  // Interface for message (de)serialization.
  std::unique_ptr<SerializerInterface> serializer;
  // Publisher for serialized messages.
  std::unique_ptr<internal::Publisher> pub;
};

RosPublisherSystem::RosPublisherSystem(
    std::unique_ptr<SerializerInterface> serializer,
    const std::string& topic_name, const rclcpp::QoS& qos, DrakeRos* ros,
    const std::unordered_set<drake::systems::TriggerType>& publish_triggers,
    double publish_period)
    : impl_(new Impl()) {
  impl_->serializer = std::move(serializer);

  impl_->pub = std::make_unique<internal::Publisher>(
      ros->get_mutable_node()->get_node_base_interface().get(),
      *impl_->serializer->GetTypeSupport(), topic_name, qos);

  DeclareAbstractInputPort("message",
                           *(impl_->serializer->CreateDefaultValue()));

  // vvv Mostly copied from LcmPublisherSystem vvv
  // Check that publish_triggers does not contain an unsupported trigger.
  for (const auto& trigger : publish_triggers) {
    if ((trigger != drake::systems::TriggerType::kForced) &&
        (trigger != drake::systems::TriggerType::kPeriodic) &&
        (trigger != drake::systems::TriggerType::kPerStep)) {
      throw std::invalid_argument(
          "Only kForced, kPeriodic, or kPerStep are supported");
    }
  }

  // Declare a forced publish so that any time Publish(.) is called on this
  // system (or a Diagram containing it), a message is emitted.
  if (publish_triggers.find(drake::systems::TriggerType::kForced) !=
      publish_triggers.end()) {
    this->DeclareForcedPublishEvent(&RosPublisherSystem::PublishInput);
  }

  if (publish_triggers.find(drake::systems::TriggerType::kPeriodic) !=
      publish_triggers.end()) {
    if (publish_period <= 0.0) {
      throw std::invalid_argument("kPeriodic requires publish_period > 0");
    }
    const double offset = 0.0;
    this->DeclarePeriodicPublishEvent(publish_period, offset,
                                      &RosPublisherSystem::PublishInput);
  } else if (publish_period > 0) {
    // publish_period > 0 without drake::systems::TriggerType::kPeriodic has no
    // meaning and is likely a mistake.
    throw std::invalid_argument("publish_period > 0 requires kPeriodic");
  }

  if (publish_triggers.find(drake::systems::TriggerType::kPerStep) !=
      publish_triggers.end()) {
    DeclarePerStepEvent(drake::systems::PublishEvent<double>(
        [this](const drake::systems::Context<double>& context,
               const drake::systems::PublishEvent<double>&) {
          PublishInput(context);
        }));
  }
  // ^^^ Mostly copied from LcmPublisherSystem ^^^
}

RosPublisherSystem::~RosPublisherSystem() {}

void RosPublisherSystem::Publish(
    const rclcpp::SerializedMessage& serialized_msg) {
  impl_->pub->publish(serialized_msg);
}

drake::systems::EventStatus RosPublisherSystem::PublishInput(
    const drake::systems::Context<double>& context) const {
  const drake::AbstractValue& input =
      get_input_port().Eval<drake::AbstractValue>(context);
  impl_->pub->publish(impl_->serializer->Serialize(input));
  return drake::systems::EventStatus::Succeeded();
}
}  // namespace drake_ros_core
