# Using External ROS Resources in Drake

This example shows how to use external ROS resources, such as URDF files in Drake.
It uses the [`ament_cmake`](https://index.ros.org/doc/ros2/Tutorials/Ament-CMake-Documentation/)
build system and [`colcon`](https://colcon.readthedocs.io) command line tool, 
with an installed instance of the Drake [binary packages](https://drake.mit.edu/from_binary.html).

## Instructions

To use `ament_cmake` and `colcon` from the ROS 2 Dashing package archive, install
the required packages and configure your environment as follows:
```
sudo ../../scripts/setup/linux/ubuntu/bionic/install_prereqs --ros-dashing
source /opt/ros/dashing/setup.bash
```

To build the `drake_example_pendulum` example:
```
colcon build --cmake-args "-DCMAKE_PREFIX_PATH=/path/to/drake;$CMAKE_PREFIX_PATH"
```

*If the Drake binary package is installed to `/opt/drake`, you may omit the
`--cmake-args <args>` argument.*


To run the `drake_example_pendulum` example, source the workspace and run the
executable:

```
source install/setup.bash
drake_example_pendulum
```
