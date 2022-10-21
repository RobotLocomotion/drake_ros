# Drake ROS

[![drake-ros continuous integration](https://github.com/RobotLocomotion/drake-ros/actions/workflows/main.yml/badge.svg?branch=main)](https://github.com/RobotLocomotion/drake-ros/actions/workflows/main.yml?query=branch%3Amain)

## About

The intended function of this repository is to provide the following ROS 2
capability:

 - API for integration between Drake and ROS 2 components
 - Bazel Starlark macros and tooling to enable ingesting (already built) ROS 2
   workspaces from either installed locations or tarballs.
 - Examples for using these APIs and Bazel macros.

In addition to ROS 2, this repository also provides some tools for interacting
with the Gazebo and broader ROS ecosystem:

- An example of fetching Gazebo or ROS models, rendering them, and ensuring
  mesh and geometry offsets are consistently interpreted between Drake and
  Gazebo. This may adjust the constituent URDF and/or SDFormat models.

## Usable! But No Stability Commitment

This code is prioritized for use within the TRI Dexterous Manipulation Group.
We do not presently adhere to any stability commitment (e.g., deprecations,
backports, etc.), so please use this code at your own discretion. (It's still
great code, of course!)

Please note that this should be considered *second-party* to Drake; more
specifically:

- Only a small subset of Drake developers are directly involved in this project
  at present.
- This does not go through the same level of review as Drake's code review.
- This project does not yet align with Drake's release cycles or [Drake's
  supported configurations](https://drake.mit.edu/from_source.html#supported-configurations).

## Contributing

See [CONTRIBUTING](./CONTRIBUTING.md) documentation.
