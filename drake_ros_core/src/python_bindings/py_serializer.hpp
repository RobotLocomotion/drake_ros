// Copyright 2020 Open Source Robotics Foundation, Inc.
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
#ifndef PYTHON_BINDINGS__PY_SERIALIZER_HPP_
#define PYTHON_BINDINGS__PY_SERIALIZER_HPP_

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <rmw/rmw.h>

#include <memory>

#include "drake_ros_core/serializer_interface.hpp"

namespace py = pybind11;

namespace drake_ros_core
{
// Serialize/Deserialize Python ROS types to rclcpp::SerializedMessage
class PySerializer : public SerializerInterface
{
public:
  explicit PySerializer(py::object message_type)
  : message_type_(message_type)
  {
    py::dict scope;
    py::exec(
      R"delim(
def get_typesupport(msg_type, attribute_name):
    metaclass = msg_type.__class__
    if metaclass._TYPE_SUPPORT is None:
        # Import typesupport if not done already
        metaclass.__import_type_support__()
    return getattr(metaclass, attribute_name)


def make_abstract_value(some_type):
    from pydrake.common.value import AbstractValue
    return AbstractValue.Make(some_type())
      )delim",
      py::globals(), scope);

    py_make_abstract_value_ = scope["make_abstract_value"];
    py::object py_get_typesupport = scope["get_typesupport"];

    // Get type support capsule and pointer
    auto typesupport = py::cast<py::capsule>(py_get_typesupport(message_type, "_TYPE_SUPPORT"));
    // TODO(sloretz) use get_pointer() in py 2.6+
    type_support_ = static_cast<decltype(type_support_)>(typesupport);

    auto convert_from_py =
      py::cast<py::capsule>(py_get_typesupport(message_type, "_CONVERT_FROM_PY"));
    // TODO(sloretz) use get_pointer() in py 2.6+
    convert_from_py_ = reinterpret_cast<decltype(convert_from_py_)>(
      static_cast<void *>(convert_from_py));

    auto convert_to_py =
      py::cast<py::capsule>(py_get_typesupport(message_type, "_CONVERT_TO_PY"));
    // TODO(sloretz) use get_pointer() in py 2.6+
    convert_to_py_ = reinterpret_cast<decltype(convert_to_py_)>(
      static_cast<void *>(convert_to_py));

    auto create_ros_message =
      py::cast<py::capsule>(py_get_typesupport(message_type, "_CREATE_ROS_MESSAGE"));
    // TODO(sloretz) use get_pointer() in py 2.6+
    create_ros_message_ = reinterpret_cast<decltype(create_ros_message_)>(
      static_cast<void *>(create_ros_message));

    auto destroy_ros_message =
      py::cast<py::capsule>(py_get_typesupport(message_type, "_DESTROY_ROS_MESSAGE"));
    // TODO(sloretz) use get_pointer() in py 2.6+
    destroy_ros_message_ = reinterpret_cast<decltype(destroy_ros_message_)>(
      static_cast<void *>(destroy_ros_message));
  }

  rclcpp::SerializedMessage
  serialize(const drake::AbstractValue & abstract_value) const override
  {
    // convert from inaccessible drake::pydrake::Object type
    py::dict scope;
    scope["av"] = &abstract_value;
    py::object message = py::eval("av.Clone().get_mutable_value()", scope);

    // Create  C ROS message
    auto c_ros_message = std::unique_ptr<void, decltype(destroy_ros_message_)>(
      create_ros_message_(), destroy_ros_message_);

    // Convert the Python message to a C ROS message
    if (!convert_from_py_(message.ptr(), c_ros_message.get())) {
      throw std::runtime_error("Failed to convert Python message to C ROS message");
    }

    // Serialize the C message
    rclcpp::SerializedMessage serialized_msg;
    const auto ret = rmw_serialize(
      c_ros_message.get(),
      type_support_,
      &serialized_msg.get_rcl_serialized_message());
    if (ret != RMW_RET_OK) {
      throw std::runtime_error("Failed to serialize C ROS message");
    }
    return serialized_msg;
  }

  bool
  deserialize(
    const rclcpp::SerializedMessage & serialized_message,
    drake::AbstractValue & abstract_value) const override
  {
    // TODO(sloretz) it would be so much more convenient if I didn't have to
    // care that the Python typesupport used the C type support internally.
    // Why isn't this encapsulated in the python type support itself?

    // Create a C type support version of the message
    auto c_ros_message = std::unique_ptr<void, decltype(destroy_ros_message_)>(
      create_ros_message_(), destroy_ros_message_);
    if (nullptr == c_ros_message.get()) {
      return false;
    }

    // Deserialize to C message type
    const auto ret = rmw_deserialize(
      &serialized_message.get_rcl_serialized_message(),
      type_support_,
      c_ros_message.get());

    if (RMW_RET_OK != ret) {
      return false;
    }

    // Convert C type to Python type
    PyObject * pymessage = convert_to_py_(c_ros_message.get());
    if (!pymessage) {
      return false;
    }

    // Store the Python message in the AbstractValue
    // convert to inaccessible drake::pydrake::Object type
    py::dict scope;
    scope["av"] = &abstract_value;
    scope["message"] = pymessage;
    py::exec("av.set_value(message)", scope);

    return true;
  }

  std::unique_ptr<drake::AbstractValue>
  create_default_value() const override
  {
    return py::cast<std::unique_ptr<drake::AbstractValue>>(py_make_abstract_value_(message_type_));
  }

  const rosidl_message_type_support_t *
  get_type_support() const override
  {
    return type_support_;
  }

private:
  py::object message_type_;
  rosidl_message_type_support_t * type_support_;

  py::object py_make_abstract_value_;

  bool (* convert_from_py_)(PyObject *, void *);
  PyObject * (* convert_to_py_)(void *);
  void * (* create_ros_message_)(void);
  void (* destroy_ros_message_)(void *);
};
}  // namespace drake_ros_core
#endif  // PYTHON_BINDINGS__PY_SERIALIZER_HPP_
