## Example ROS 2 apps

This package exercises `rosidl` interface generation and usage in C++ and Python, with cross-package interface dependencies.

To check that the resulting interfaces are functional, it includes a demo applications in C++ and Python. Namely:

- Equivalent `oracle_(cc|py)` applications that publish `ros2_example_apps_msgs/msg/Status` messages, serve a `ros2_example_common_msgs/srv/Query` service, and provide a `ros2_example_common_msgs/action/Do` action.
- Equivalent `inquirer_(cc|py)` applications that subscribe to `ros2_example_apps_msgs/msg/Status` messages, make `ros2_example_common_msgs/srv/Query` service requests, and invoke the `ros2_example_common_msgs/action/Do` action.

You may run a C++ oracle against a Python inquirer, and vice versa:

```sh
bazel run //ros2_example_apps:oracle_py
```

```sh
bazel run //ros2_example_apps:inquirer_cc
```
