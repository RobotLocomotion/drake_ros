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

#include "drake_ros_core/ros_subscriber_system.h"

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "subscription.h"  // NOLINT(build/include)
#include <drake/systems/framework/abstract_values.h>

namespace drake_ros_core {
namespace {
// A synchronized queue of `MessageT` messages.
template <typename MessageT>
class MessageQueue {
 public:
  void PutMessage(std::shared_ptr<MessageT> message) {
    std::lock_guard<std::mutex> lock(mutex_);
    message_ = message;
  }

  std::shared_ptr<MessageT> TakeMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::move(message_);
  }

 private:
  // Mutex to synchronize access to the queue.
  std::mutex mutex_;
  // Last received message (i.e. queue of size 1).
  std::shared_ptr<MessageT> message_;
};
}  // namespace

struct RosSubscriberSystem::Impl {
  // Interface for message (de)serialization.
  std::unique_ptr<SerializerInterface> serializer;
  // Subscription to serialized messages.
  std::shared_ptr<internal::Subscription> sub;
  // Queue of serialized messages.
  MessageQueue<rclcpp::SerializedMessage> queue;
  // AbstractState index where the message is stored.
  drake::systems::AbstractStateIndex message_state_index;
};

RosSubscriberSystem::RosSubscriberSystem(
    std::unique_ptr<SerializerInterface> serializer,
    const std::string& topic_name, const rclcpp::QoS& qos, DrakeRos* ros)
    : impl_(new Impl()) {
  impl_->serializer = std::move(serializer);

  rclcpp::Node* node = ros->get_mutable_node();
  impl_->sub = std::make_shared<internal::Subscription>(
      node->get_node_base_interface().get(),
      *impl_->serializer->GetTypeSupport(), topic_name, qos,
      std::bind(&MessageQueue<rclcpp::SerializedMessage>::PutMessage,
                &impl_->queue, std::placeholders::_1));
  node->get_node_topics_interface()->add_subscription(impl_->sub, nullptr);

  impl_->message_state_index =
      DeclareAbstractState(*(impl_->serializer->CreateDefaultValue()));

  DeclareStateOutputPort(drake::systems::kUseDefaultName,
                         impl_->message_state_index);
}

RosSubscriberSystem::~RosSubscriberSystem() {}

void RosSubscriberSystem::DoCalcNextUpdateTime(
    const drake::systems::Context<double>& context,
    drake::systems::CompositeEventCollection<double>* events,
    double* time) const {
  // Vvv Copied from LcmSubscriberSystem vvv

  // We do not support events other than our own message timing events.
  LeafSystem<double>::DoCalcNextUpdateTime(context, events, time);
  DRAKE_THROW_UNLESS(events->HasEvents() == false);
  DRAKE_THROW_UNLESS(std::isinf(*time));

  std::shared_ptr<rclcpp::SerializedMessage> message =
      impl_->queue.TakeMessage();

  // Do nothing unless we have a new message.
  if (!message) {
    return;
  }

  // Create a unrestricted event and tie the handler to the corresponding
  // function.
  drake::systems::UnrestrictedUpdateEvent<double>::UnrestrictedUpdateCallback
      callback = [this, serialized_message{std::move(message)}](
                     const drake::systems::Context<double>&,
                     const drake::systems::UnrestrictedUpdateEvent<double>&,
                     drake::systems::State<double>* state) {
        // Deserialize the message and store it in the abstract state on the
        // context
        drake::systems::AbstractValues& abstract_state =
            state->get_mutable_abstract_state();
        auto& abstract_value =
            abstract_state.get_mutable_value(impl_->message_state_index);
        impl_->serializer->Deserialize(*serialized_message, &abstract_value);
        return drake::systems::EventStatus::Succeeded();
      };

  // Schedule an update event at the current time.
  *time = context.get_time();
  drake::systems::EventCollection<
      drake::systems::UnrestrictedUpdateEvent<double>>& uu_events =
      events->get_mutable_unrestricted_update_events();
  uu_events.AddEvent(drake::systems::UnrestrictedUpdateEvent<double>(
      drake::systems::TriggerType::kTimed, callback));
}
}  // namespace drake_ros_core
