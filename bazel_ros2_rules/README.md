# ROS 2 rules for Bazel

This project provides rules to build and run against a ROS 2 distribution from Bazel.

## Features

- Automatic ROS 2 overlay scraping, `symlink` or `merge`-installed
- ROS 2 aware C++/Python binaries (e.g. `dload` capable)
- ROS 2 interface generation within Bazel

## Platform support

These rules require ROS 2 Rolling distributions on Ubuntu Focal 20.04 and onwards.

## Usage

1. Add `bazel_ros2_rules` to your WORKSPACE (e.g. via `http_archive()`).

1. Add `bazel_ros2_rules` dependencies to you WORKSPACE:

```starlark
load("@bazel_ros2_rules//deps:defs.bzl", "add_bazel_ros2_rules_dependencies")
add_bazel_ros2_rules_dependencies()
```

1. Bind a local ROS 2 workspace underlay in your WORKSPACE:

```starlark
load("@bazel_ros2_rules//ros2:defs.bzl", "ros2_local_repository")
ros2_local_repository(
    name = "ros2",
    workspace = ["/opt/ros/<distro>"],
)
```

For further documentation on available rules, refer to `ros2` package [documentation](ros2/README.md).
