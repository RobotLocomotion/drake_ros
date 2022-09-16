# IIWA Manipulator

## Overview

Both `iiwa_manipulator` and `iiwa_manipulator.py` enable RViz visualization of a static [`ManipulationStation`](https://github.com/RobotLocomotion/drake/tree/master/examples/manipulation_station) example.
They publish the following topics:

* `/tf` (all scene frames)
* `/scene_markers` (all scene geometries, including the robot model)

## How To

Run either the C++ `iiwa_manipulator` executable or the Python `iiwa_manipulator.py` script.
For the C++ version of the example, run the executable.

```
ros2 run drake_ros_examples iiwa_manipulator
```

For the Python version of the example, run the Python script.

```
ros2 run drake_ros_examples iiwa_manipulator.py
```

Run RViz in a different terminal with your ROS installation sourced to visualize the station:

```
ros2 run rviz2 rviz2 -d iiwa_manipulator.rviz
```
