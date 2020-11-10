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
#ifndef PYTHON_BINDINGS__QOS_TYPE_CASTER_HPP_
#define PYTHON_BINDINGS__QOS_TYPE_CASTER_HPP_

#include <pybind11/pybind11.h>

#include <rclcpp/qos.hpp>

#include <memory>

namespace drake_ros_core
{
// Thin wrapper that adds default constructor
class QoS : public rclcpp::QoS
{
public:
  QoS()
  : rclcpp::QoS(1) {}
};
}  // namespace drake_ros_core

namespace pybind11
{
namespace detail
{
template<>
struct type_caster<drake_ros_core::QoS>
{
public:
  // declares local 'value' of type rclcpp::QoS
  PYBIND11_TYPE_CASTER(drake_ros_core::QoS, _("rclpy.qos.QoSProfile"));

  // Convert from Python rclpy.qos.QoSProfile to rclcpp::QoS
  bool load(handle src, bool)
  {
    /* Extract PyObject from handle */
    PyObject * source = src.ptr();

    // history                          : enum int
    // depth                            : int
    // reliability                      : enum int
    // durability                       : enum int
    // lifespan                         : rclpy.Duration
    // deadline                         : rclpy.Duration
    // liveliness                       : enum int
    // liveliness_lease_duration        : rclpy.Duration
    // avoid_ros_namespace_conventions  : bool

    PyObject * py_history = nullptr;
    PyObject * py_depth = nullptr;
    PyObject * py_reliability = nullptr;
    PyObject * py_durability = nullptr;
    PyObject * py_lifespan = nullptr;
    PyObject * py_lifespan_ns = nullptr;
    PyObject * py_deadline = nullptr;
    PyObject * py_deadline_ns = nullptr;
    PyObject * py_liveliness = nullptr;
    PyObject * py_liveliness_lease_duration = nullptr;
    PyObject * py_liveliness_lease_duration_ns = nullptr;
    PyObject * py_avoid_ros_namespace_conventions = nullptr;

    // Clean up references when function exits.
    std::unique_ptr<void, std::function<void(void *)>> scope_exit(
      nullptr, [&](void *) {
        Py_XDECREF(py_avoid_ros_namespace_conventions);
        Py_XDECREF(py_liveliness_lease_duration_ns);
        Py_XDECREF(py_liveliness_lease_duration);
        Py_XDECREF(py_liveliness);
        Py_XDECREF(py_deadline_ns);
        Py_XDECREF(py_deadline);
        Py_XDECREF(py_lifespan_ns);
        Py_XDECREF(py_lifespan);
        Py_XDECREF(py_durability);
        Py_XDECREF(py_reliability);
        Py_XDECREF(py_depth);
        Py_XDECREF(py_history);
      });

    size_t history;
    size_t depth;
    size_t reliability;
    size_t durability;
    size_t lifespan_ns;
    size_t deadline_ns;
    size_t liveliness;
    size_t liveliness_lease_duration_ns;
    int avoid_ros_namespace_conventions;

    // Get all the QoS Attributes
    py_history = PyObject_GetAttrString(source, "history");
    if (!py_history) {
      return false;
    }
    py_depth = PyObject_GetAttrString(source, "depth");
    if (!py_depth) {
      return false;
    }
    py_reliability = PyObject_GetAttrString(source, "reliability");
    if (!py_reliability) {
      return false;
    }
    py_durability = PyObject_GetAttrString(source, "durability");
    if (!py_durability) {
      return false;
    }
    py_lifespan = PyObject_GetAttrString(source, "lifespan");
    if (!py_lifespan) {
      return false;
    }
    py_deadline = PyObject_GetAttrString(source, "deadline");
    if (!py_deadline) {
      return false;
    }
    py_liveliness = PyObject_GetAttrString(source, "liveliness");
    if (!py_liveliness) {
      return false;
    }
    py_liveliness_lease_duration =
      PyObject_GetAttrString(source, "liveliness_lease_duration");
    if (!py_liveliness_lease_duration) {
      return false;
    }
    py_avoid_ros_namespace_conventions =
      PyObject_GetAttrString(source, "avoid_ros_namespace_conventions");
    if (!py_avoid_ros_namespace_conventions) {
      return false;
    }

    // Do Type Conversions
    // History and depth if history is keep_last
    history = PyNumber_AsSsize_t(py_history, NULL);
    switch (history) {
      case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
        depth = PyNumber_AsSsize_t(py_depth, PyExc_OverflowError);
        if (PyErr_Occurred()) {
          return false;
        }
        value.keep_last(depth);
        break;
      case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      case RMW_QOS_POLICY_HISTORY_UNKNOWN:
        value.history(static_cast<rmw_qos_history_policy_t>(history));
        break;
      default:
        if (!PyErr_Occurred()) {
          PyErr_Format(PyExc_ValueError, "Unsupported history policy %zu", history);
        }
        return false;
    }

    // Reliability
    reliability = PyNumber_AsSsize_t(py_reliability, NULL);
    switch (reliability) {
      case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      case RMW_QOS_POLICY_RELIABILITY_UNKNOWN:
        value.reliability(static_cast<rmw_qos_reliability_policy_t>(reliability));
        break;
      default:
        if (!PyErr_Occurred()) {
          PyErr_Format(PyExc_ValueError, "Unsupported reliability policy %zu", reliability);
        }
        return false;
    }

    // Durability
    durability = PyNumber_AsSsize_t(py_durability, NULL);
    switch (durability) {
      case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      case RMW_QOS_POLICY_DURABILITY_UNKNOWN:
        value.durability(static_cast<rmw_qos_durability_policy_t>(durability));
        break;
      default:
        if (!PyErr_Occurred()) {
          PyErr_Format(PyExc_ValueError, "Unsupported durability policy %zu", durability);
        }
        return false;
    }

    // lifespan
    py_lifespan_ns = PyObject_GetAttrString(py_lifespan, "nanoseconds");
    if (!py_lifespan_ns) {
      return false;
    }
    lifespan_ns = PyNumber_AsSsize_t(py_lifespan_ns, PyExc_OverflowError);
    if (PyErr_Occurred()) {
      return false;
    }
    value.lifespan(rclcpp::Duration::from_nanoseconds(lifespan_ns));

    // deadline
    py_deadline_ns = PyObject_GetAttrString(py_deadline, "nanoseconds");
    if (!py_deadline_ns) {
      return false;
    }
    deadline_ns = PyNumber_AsSsize_t(py_deadline_ns, PyExc_OverflowError);
    if (PyErr_Occurred()) {
      return false;
    }
    value.deadline(rclcpp::Duration::from_nanoseconds(deadline_ns));

    // liveliness
    liveliness = PyNumber_AsSsize_t(py_liveliness, NULL);
    switch (liveliness) {
      case RMW_QOS_POLICY_LIVELINESS_AUTOMATIC:
      case RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC:
      case RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT:
      case RMW_QOS_POLICY_LIVELINESS_UNKNOWN:
        value.liveliness(static_cast<rmw_qos_liveliness_policy_t>(liveliness));
        break;
      default:
        if (!PyErr_Occurred()) {
          PyErr_Format(PyExc_ValueError, "Unsupported liveliness policy %zu", liveliness);
        }
        return false;
    }

    // liveliness_lease_duration
    py_liveliness_lease_duration_ns = PyObject_GetAttrString(
      py_liveliness_lease_duration, "nanoseconds");
    if (!py_liveliness_lease_duration_ns) {
      return false;
    }
    liveliness_lease_duration_ns = PyNumber_AsSsize_t(
      py_liveliness_lease_duration_ns, PyExc_OverflowError);
    if (PyErr_Occurred()) {
      return false;
    }
    value.liveliness_lease_duration(
      rclcpp::Duration::from_nanoseconds(liveliness_lease_duration_ns));

    // avoid_ros_namespace_conventions
    avoid_ros_namespace_conventions = PyObject_IsTrue(py_avoid_ros_namespace_conventions);
    if (-1 == avoid_ros_namespace_conventions) {
      return false;
    }
    value.avoid_ros_namespace_conventions(avoid_ros_namespace_conventions);

    return true;
  }

  // Convert from Python rclcpp::QoS to rclpy.qos.QoSProfile
  static handle cast(drake_ros_core::QoS src, return_value_policy policy, handle parent)
  {
    (void) src;
    (void) policy;
    (void) parent;
    Py_RETURN_NOTIMPLEMENTED;
  }
};
}  // namespace detail
}  // namespace pybind11
#endif  // PYTHON_BINDINGS__QOS_TYPE_CASTER_HPP_
