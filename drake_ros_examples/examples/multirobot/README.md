# Multirobot

## Overview

The `multirobot.py` script enables RViz visualisation of an array of Kuka LBR iiwa manipulators.
It publishes the following topics:

* `/tf` (all scene frames)
* `/` ()

## How to run the example

Run the Python `multirobot.py` script as explained [here](../../README.md#running).

In a separate terminal, launch RViz and provide the path to the configuration file to visualise the robots.

```
ros2 run rviz2 rviz2 -d $(ros2 pkg prefix drake_ros_examples)/share/drake_ros_examples/multirobot.rviz
```
