# IIWA Manipulator

## Overview

Both `iiwa_manipulator` and `iiwa_manipulator.py` enable RViz visualization of a static [`ManipulationStation`](https://github.com/RobotLocomotion/drake/tree/master/examples/manipulation_station) example.
They publish the following topics:

* `/tf` (all scene frames)
* `/scene_markers` (all scene geometries, including the robot model)

## How To

Run either the C++ `iiwa_manipulator` executable or the Python `iiwa_manipulator.py` script as explained [here](../../README.md#running).

Run RViz in a different terminal with your ROS installation sourced to visualize the station:

```
ros2 run rviz2 rviz2 -d iiwa_manipulator.rviz
```


