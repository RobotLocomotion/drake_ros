#!/usr/bin/env python3

import argparse
import json
import pathlib
import subprocess
import sys


# The list of rosdep keys that are skipped has been taken verbatim from
# ROS 2 Rolling binary install docs.
#
# This is necessary because:
# - Some non-ROS packages don't always install their package manifests
#   (cyclonedds, fastcdr, fastrtps, iceoryx_binding_c, urdfdom_headers)
# - Group dependencies aren't supported everywhere and are hard-coded in
#   some packages (rti-connext-dds-5.3.1)
#
# See https://docs.ros.org/en/rolling/Installation/Ubuntu-Install-Binary.html
# for further reference.
SKIPPED_ROSDEP_KEYS = {
    'cyclonedds', 'fastcdr', 'fastrtps', 'iceoryx_binding_c',
    'rti-connext-dds-5.3.1', 'urdfdom_headers'}


def parse_arguments():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        '-o', '--output',
        type=argparse.FileType('w'), default=sys.stdout,
        help='Path to file to write BUILD.bazel content to'
    )
    parser.add_argument(
        'workspace_paths', type=pathlib.Path, nargs='+',
        help='Paths to (potentially overlayed) workspaces'
    )
    args = parser.parse_args()

    return args


def compute_system_rosdeps(workspace_paths):
    share_paths = [  # leverage REP-122
        path for workspace_path in workspace_paths
        for path in workspace_path.glob('**/share')
    ]

    # Run a check to see if the rosdep database is ready to be used
    cmd = ['rosdep', 'check', '--from-paths', share_paths[0]]
    try:
        subprocess.check_output(
            cmd, env={'ROS_PYTHON_VERSION': '3'},
            stderr=subprocess.PIPE,
            encoding='utf-8')
    except subprocess.CalledProcessError as e:
        if e.stderr and 'rosdep init' in e.stderr:
            raise RuntimeError('The rosdep database is not initalized. '
                    'Please run `rosdep init`')
        if e.stderr and 'rosdep update' in e.stderr:
            # Run this for the user
            subprocess.check_output(
                ['rosdep', 'update'], env={'ROS_PYTHON_VERSION': '3'},
                encoding='utf-8')
        else:
            raise RuntimeError(f'An unknown error has occured: {e}')

    cmd = [
        'rosdep', 'keys', '-i',
        '-t', 'buildtool_export',
        '-t', 'build_export',
        '-t', 'exec', '--from-paths', *share_paths
    ]
    output = subprocess.check_output(
        cmd, env={'ROS_PYTHON_VERSION': '3'},
        encoding='utf-8')
    rosdep_keys = set(output.splitlines())
    return sorted(rosdep_keys - SKIPPED_ROSDEP_KEYS)


def main():
    args = parse_arguments()

    system_rosdeps = compute_system_rosdeps(args.workspace_paths)
    args.output.write('\n'.join(system_rosdeps) + '\n')


if __name__ == '__main__':
    main()
