# Bazel Project with ROS 2 as a Precompiled External

This project builds C++ and Python examples against a system-installed
ROS 2 Rolling binary distribution within a Bazel workspace.

**Note**: At present, TRI's Drake users are on Ubuntu Focal for the medium term. 
This project deals with that constrain by installing a ROS 2 Rolling variant
that runs on Ubuntu Focal from a tarball. See [`setup/install_prereqs.sh`](./setup/install_prereqs.sh).

For an introduction to Bazel, refer to [Getting Started with Bazel](https://docs.bazel.build/versions/master/getting-started.html).

For documentation on the underlying Bazel infrastructure, refer to
[`bazel_ros2_rules`](../bazel_ros2_rules/README.md).

## Instructions

First, install the required dependencies:

```sh
sudo ./setup/install_prereqs.sh
```

To build all packages:

```sh
bazel build //...
```

To run examples binaries directly:

```sh
bazel run //ros2_example_apps:oracle_cc
```

```sh
bazel run //ros2_example_apps:inquirer_py
```

You may also run tests:

```sh
bazel test //...
```
