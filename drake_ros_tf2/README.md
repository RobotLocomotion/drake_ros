# Drake ROS TF2

This package provides abstractions to simplify the use of tf2 ROS functionality from within Drake.

## Building

This package has been built and tested on Ubuntu Focal with ROS Rolling, using a Drake nightly or any stable releses after 14 Jan 2022.
It may work on other versions of ROS and Drake, but it hasn't been tested.

To build it:

1. [Install ROS Rolling](https://index.ros.org/doc/ros2/Installation/Rolling/)
1. Source your ROS installation `. /opt/ros/rolling/setup.bash`
1. [Download Drake binary](https://drake.mit.edu/from_binary.html), nightly or any stable releases after 14 Jan 2022.
1. Extract the Drake binary installation, install it's prerequisites, and [use this Python virutalenv trick](https://drake.mit.edu/from_binary.html).
1. Activate the Drake virtual environment.
1. Build it using Colcon.

    **Colcon**
    1. Make a workspace `mkdir -p ./ws/src`
    1. `cd ./ws/src`
    1. Get this code `git clone https://github.com/RobotLocomotion/drake-ros.git`
    1. `cd ..`
    1. Build this package and its dependencies `colcon build --packages-up-to drake_ros_tf2`
