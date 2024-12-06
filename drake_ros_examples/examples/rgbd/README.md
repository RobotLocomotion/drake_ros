# RGBD camera

## Overview

The `rgbd` executable and `rgbd.py` script enable a cartpole simulation with a RGBD sensor looking at it

The simulation publishes the following topics:

 * `/tf` (all scene frames)
 * `/color/image_raw`
 * `/color/camera_info`
 * `/depth/image_raw`
 * `/depth/camera_info`

## How To

```sh
# Using Colcon/CMake
ros2 run rviz2 rviz2 -d $(ros2 pkg prefix drake_ros_examples)/share/drake_ros_examples/rgbd/rgbd.rviz

# Using bazel
bazel run @ros2//:rviz2 -- -d `pwd`/examples/rgbd/rgbd.rviz
```

In a separate terminal, run either the C++ executable or the Python script.

# Using Colcon/CMake
# C++
ros2 run drake_ros_examples rgbd
# Python
ros2 run drake_ros_examples rgbd.py

# Using bazel
# C++
bazel run //examples/rgbd:rgbd
# Python
bazel run //examples/rgbd:rgbd_py
```

Or use ROS 2 Launch:

```bash
ros2 launch drake_ros_examples rgba_launch.py
```
