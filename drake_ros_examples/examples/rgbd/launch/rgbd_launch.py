#!/bin/python
import os

from ament_index_python.packages import get_package_prefix
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import ExecuteProcess

import launch_ros.actions
import launch_ros.descriptions


def generate_launch_description():
    rviz_config_file = os.path.join(get_package_share_directory('drake_ros_examples'),
                                    'rgbd/rgbd.rviz')

    rviz_node = ExecuteProcess(
        cmd=['rviz2', '-d', rviz_config_file],
        output='screen'
    )

    rgbd_node = ExecuteProcess(
        cmd=[os.path.join(get_package_prefix('drake_ros_examples'), 'lib',
                          'drake_ros_examples', 'rgbd')],
    )

    return LaunchDescription([
        rviz_node,
        rgbd_node,
    ])
