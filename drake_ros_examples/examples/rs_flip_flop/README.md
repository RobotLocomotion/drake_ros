# RS flip flop

## Overview

Both `rs_flip_flop` and `rs_flip_flop.py` implement an [RS flip flop using NOR gates](https://en.wikipedia.org/wiki/Flip-flop_(electronics)#SR_NOR_latch).
They subscribe to the following topics:

* `/R`
* `/S`

And publish to the following topics

* `/Q`
* `/Q_not`

## How To

Run either the C++ `rs_flip_flop` executable or the Python `rs_flip_flop.py` script as explained [here](../../README.md#running).

Run these commands in different terminals with your ROS installation sourced to echo the output topics:

```
ros2 topic echo /Q
```

```
ros2 topic echo /Q_not
```

Run these commands in different terminals with your ROS installation sourced to play with the input topics.

```
ros2 topic pub /S std_msgs/msg/Bool "data: false"
ros2 topic pub /S std_msgs/msg/Bool "data: true"
```

```
ros2 topic pub /R std_msgs/msg/Bool "data: false"
ros2 topic pub /R std_msgs/msg/Bool "data: true"
```
