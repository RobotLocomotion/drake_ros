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

#include <algorithm>
#include <memory>
#include <string>

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>

#include "drake_ros_core/serializer_interface.h"

namespace py = pybind11;

namespace drake_ros_core {
// A (de)serialization interface implementation for Python ROS messages
// that can be overriden from Python itself.
class PySerializerInterface : public py::wrapper<SerializerInterface> {
 public:
  using Base = py::wrapper<SerializerInterface>;

  PySerializerInterface() : Base() {}

  const rosidl_message_type_support_t* GetTypeSupport() const override {
    auto overload = [&]() -> py::capsule {
      PYBIND11_OVERLOAD_PURE(py::capsule, SerializerInterface, GetTypeSupport);
    };
    return static_cast<rosidl_message_type_support_t*>(overload());
  }

  std::unique_ptr<drake::AbstractValue> CreateDefaultValue() const override {
    PYBIND11_OVERLOAD_PURE(std::unique_ptr<drake::AbstractValue>,
                           SerializerInterface, CreateDefaultValue);
  }

  rclcpp::SerializedMessage Serialize(
      const drake::AbstractValue& abstract_value) const override {
    auto overload = [&]() -> py::bytes {
      PYBIND11_OVERLOAD_PURE(py::bytes, SerializerInterface, Serialize,
                             &abstract_value);
    };
    std::string bytes = overload();
    rclcpp::SerializedMessage serialized_message(bytes.size());
    rcl_serialized_message_t& rcl_serialized_message =
        serialized_message.get_rcl_serialized_message();
    std::copy(bytes.data(), bytes.data() + bytes.size(),
              rcl_serialized_message.buffer);
    rcl_serialized_message.buffer_length = bytes.size();
    return serialized_message;
  }

  void Deserialize(const rclcpp::SerializedMessage& serialized_message,
                   drake::AbstractValue* abstract_value) const override {
    const rcl_serialized_message_t& rcl_serialized_message =
        serialized_message.get_rcl_serialized_message();
    py::bytes serialized_message_bytes(
        reinterpret_cast<const char*>(rcl_serialized_message.buffer),
        rcl_serialized_message.buffer_length);
    PYBIND11_OVERLOAD_PURE(void, SerializerInterface, Deserialize,
                           serialized_message_bytes, abstract_value);
  }
};
}  // namespace drake_ros_core
