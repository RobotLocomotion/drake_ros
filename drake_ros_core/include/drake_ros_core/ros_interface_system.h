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

#include <drake/systems/framework/leaf_system.h>

#include "drake_ros_core/drake_ros.h"

namespace drake_ros_core {
/** A system that manages a Drake ROS interface. */
class RosInterfaceSystem : public drake::systems::LeafSystem<double> {
 public:
  /** A constructor that takes ownership of the `ros` interface. */
  explicit RosInterfaceSystem(std::unique_ptr<DrakeRos> ros);

  ~RosInterfaceSystem() override;

  /** Returns a mutable reference to the underlying ROS interface. */
  DrakeRos* get_ros_interface() const;

 protected:
  void DoCalcNextUpdateTime(const drake::systems::Context<double>&,
                            drake::systems::CompositeEventCollection<double>*,
                            double*) const override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace drake_ros_core
