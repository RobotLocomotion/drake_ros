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

#include "drake_ros_core/ros_interface_system.h"

#include <limits>
#include <memory>
#include <utility>

namespace drake_ros_core {
struct RosInterfaceSystem::Impl {
  // Interface to ROS (through a node).
  std::unique_ptr<DrakeRos> ros;
};

RosInterfaceSystem::RosInterfaceSystem(std::unique_ptr<DrakeRos> ros)
    : impl_(new Impl()) {
  impl_->ros = std::move(ros);
}

RosInterfaceSystem::~RosInterfaceSystem() {}

DrakeRos* RosInterfaceSystem::get_ros_interface() const {
  return impl_->ros.get();
}

void RosInterfaceSystem::DoCalcNextUpdateTime(
    const drake::systems::Context<double>&,
    drake::systems::CompositeEventCollection<double>*, double* time) const {
  constexpr int kMaxWorkMillis = 0;  // Do not block.
  impl_->ros->Spin(kMaxWorkMillis);
  // TODO(sloretz) Lcm system pauses time if some work was done, but ROS 2 API
  // doesn't say if any work was done. How to reconcile that?
  // TODO(hidmic): test for subscription latency in context time, how does the
  // order of node spinning and message taking affects it?
  *time = std::numeric_limits<double>::infinity();
}
}  // namespace drake_ros_core
