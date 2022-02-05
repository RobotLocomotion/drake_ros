# Drake ROS Examples

This is a collection of examples built around `drake_ros_systems` C++ and Python APIs.

## Building

This package has been built and tested on Ubuntu Focal with ROS Rolling, using a Drake nightly from April 2021.
It may work on other versions of ROS and Drake, but it hasn't been tested.

To build it:

1. [Install ROS Rolling](https://index.ros.org/doc/ros2/Installation/Rolling/)
1. Source your ROS installation `. /opt/ros/rolling/setup.bash`
1. [Download April-ish 2021 Drake binary](https://drake.mit.edu/from_binary.html)
1. Extract the Drake binary installation, install it's prerequisites, and [use this Python virutalenv trick](https://drake.mit.edu/python_bindings.html#inside-virtualenv).
1. Activate the drake virtual environment
1. Build it using Colcon, or using CMake directly
    
    **Colcon**
    1. Make a workspace `mkdir -p ./ws/src`
    1. `cd ./ws/src`
    1. Get this code `git clone https://github.com/RobotLocomotion/drake-ros.git`
    1. `cd ..`
    1. Build this package and its dependencies `colcon build --packages-up-to drake_ros_systems`
    
    **CMake**
    1. Get this code `git clone https://github.com/RobotLocomotion/drake-ros.git`
    1. Build the [`drake_ros_systems`](../drake_ros_systems/README.md#building) package using CMake first.
    1. Manually set `CMAKE_PREFIX_PATH`: `export CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH:$(pwd)/drake-ros/drake_ros_systems/install`
    1. `cd drake-ros/drake_ros_examples`
    1. Make a build and install folder to avoid installing to the whole system `mkdir build install`
    1. `cd build`
    1. Configure the project `cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/../install ..`
    1. Build the project `make && make install`

## Running

* If you built with `colcon`, then source your workspace.

```
. ./ws/install/setup.bash
# Also make sure to activate drake virtual environment
```

  Now you can run C++ and Python examples using `ros2 run drake_ros_examples <example-executable-or-script>`.


* If you built with plain CMake, then source the ROS workspace and set these variables.

```
. /opt/ros/rolling/setup.bash
# Also make sure to activate drake virtual environment
# CD to repository root
export LD_LIBRARY_PATH=$(pwd)/drake_ros_systems/install/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$(pwd)/drake_ros_examples/install/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$(pwd)/drake_ros_systems/install/lib/$(python -c 'import sys; print(f"python{sys.version_info[0]}.{sys.version_info[1]}")')/site-packages:$PYTHONPATH
export PYTHONPATH=$(pwd)/drake_ros_examples/install/lib/$(python -c 'import sys; print(f"python{sys.version_info[0]}.{sys.version_info[1]}")')/site-packages:$PYTHONPATH
```

  Now you can run C++ and Python examples from the install folder using `./drake-ros/drake_ros_examples/install/lib/drake_ros_examples/<example-executable-or-script>`.

## List of examples

- [RS flip flop](./examples/rs_flip_flop): a latch with a ROS 2 topic interface.
- [IIWA manipulator](./examples/iiwa_manipulator): an RViz visualization of a static IIWA arm.
