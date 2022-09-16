# drake_ros

[![drake-ros continuous integration](https://github.com/RobotLocomotion/drake-ros/actions/workflows/main.yml/badge.svg?branch=develop)](https://github.com/RobotLocomotion/drake-ros/actions/workflows/main.yml)

## About

The intended function of this repository is to provide:

 - API for integration between Drake and ROS 2 components
 - Bazel Starlark macros and tooling to enable ingesting (already built) ROS 2
   workspaces from either installed locations or tarballs.
 - Examples for using these APIs and Bazel macros.

## Usable! But No Stability Commitment

This code is prioritized for use within the TRI Dexterous Manipulation Group.
We do not presently adhere to any stability commitment (e.g., deprecations,
backports, etc.), so please use this code at your own disgression. (It's still
great code, of course!)

Please note that this should be considered *second-party* to Drake; more
specifically:

- Only a small subset of Drake developers are directly involved in this project'
  at present.
- This does not go through the same level of review as Drake's code review.
- This project does not yet align with Drake's release cycles or [Drake's
  supported configurations](https://drake.mit.edu/from_source.html#supported-configurations).

## Contributing

### Code style

All code in this repository should generally comply with Drake's [code style
guide](https://drake.mit.edu/code_style_guide.html).
