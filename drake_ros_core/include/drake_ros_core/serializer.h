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
#pragma once

#include <memory>

#include <drake/common/value.h>
#include <rclcpp/serialization.hpp>
#include <rclcpp/serialized_message.hpp>
#include <rosidl_typesupport_cpp/message_type_support.hpp>

#include "drake_ros_core/serializer_interface.h"

namespace drake_ros_core {
/** A (de)serialization interface implementation that is
 bound to C++ ROS messages of `MessageT` type. */
template <typename MessageT>
class Serializer : public SerializerInterface {
 public:
  rclcpp::SerializedMessage Serialize(
      const drake::AbstractValue& abstract_value) const override {
    rclcpp::SerializedMessage serialized_message;
    protocol_.serialize_message(&abstract_value.get_value<MessageT>(),
                                &serialized_message);
    return serialized_message;
  }

  void Deserialize(const rclcpp::SerializedMessage& serialized_message,
                   drake::AbstractValue* abstract_value) const override {
    protocol_.deserialize_message(
        &serialized_message, &abstract_value->get_mutable_value<MessageT>());
  }

  std::unique_ptr<drake::AbstractValue> CreateDefaultValue() const override {
    return std::make_unique<drake::Value<MessageT>>(MessageT());
  }

  const rosidl_message_type_support_t* GetTypeSupport() const override {
    return rosidl_typesupport_cpp::get_message_type_support_handle<MessageT>();
  }

 private:
  rclcpp::Serialization<MessageT> protocol_;
};
}  // namespace drake_ros_core
