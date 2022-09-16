#!/usr/bin/env python3

"""
Scrapes ROS 2 workspaces and exposes their artifacts through a Bazel local repository.

This script generates:

- a BUILD.bazel file, with targets for all C/C++ libraries, Python libraries, executables, and share
  data files found in each scrapped ROS 2 package
- a distro.bzl file, with ROS 2 metadata as constants
"""

import argparse
import json
import os
import sys

from ros2bzl.resources import load_resource

import ros2bzl.sandboxing as sandboxing

from ros2bzl.utilities import interpolate
from ros2bzl.utilities import to_starlark_string_dict


def parse_arguments():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        '-o', '--output',
        type=argparse.FileType('w'), default=sys.stdout,
        help='Output file to write distro.bzl content to'
    )
    parser.add_argument(
        '-s', '--sandbox-mapping',
        action='append', dest='sandbox_mappings',
        metavar='OUTER_PATH:INNER_PATH', default=[],
        help='Path mappings for workspace sandboxing'
    )
    parser.add_argument(
        '-d', '--distro-file',
        type=argparse.FileType('r'), required=True,
        help='Path to distro metadata file, in JSON format'
    )
    parser.add_argument(
        '--default-localhost-only',
        action='store_true', required=False,
        help='Restrict ROS 2 to localhost communication by default'
    )
    parser.add_argument(
        'repository_name', help='Bazel repository name'
    )
    args = parser.parse_args()

    args.sandbox = sandboxing.make_path_mapping(
        name=args.repository_name, mapping=dict(
            entry.split(':') for entry in args.sandbox_mappings
        )
    )

    args.distro = json.load(args.distro_file)

    return args


ROSIDL_TYPESUPPORT_GROUPS = [
    'rosidl_typesupport_c_packages',
    'rosidl_typesupport_cpp_packages'
]


def generate_distro_file_content(
        repo_name, distro, sandbox, default_localhost_only
        ):
    packages = distro['packages']
    ament_prefix_paths = distro['paths']['ament_prefix']
    library_load_paths = distro['paths']['library_load']
    return interpolate(
        load_resource('templates/distro.bzl.tpl'),
        to_starlark_string_dict({
            'AMENT_PREFIX_PATHS': [
                sandbox(path, external=True) for path in ament_prefix_paths],
            'LOAD_PATHS': [
                sandbox(path, external=True) for path in library_load_paths],
            'AVAILABLE_TYPESUPPORT_LIST': [
                name for name, metadata in packages.items()
                if 'groups' in metadata and any(
                    group in ROSIDL_TYPESUPPORT_GROUPS
                    for group in metadata['groups']
                )],
            'REPOSITORY_ROOT': '@{}//'.format(repo_name),
            'DEFAULT_LOCALHOST_ONLY': '1' if default_localhost_only else '0',
        })
    ) + '\n'


def main():
    args = parse_arguments()

    args.output.write(generate_distro_file_content(
        args.repository_name, args.distro, args.sandbox,
        args.default_localhost_only))


if __name__ == '__main__':
    main()
