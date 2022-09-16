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

import toposort

from ros2bzl.resources import load_resource

import ros2bzl.sandboxing as sandboxing

from ros2bzl.scraping.ament_cmake \
    import collect_ament_cmake_package_direct_properties
from ros2bzl.scraping.ament_cmake \
    import collect_ament_cmake_package_properties
from ros2bzl.scraping.ament_cmake import precache_ament_cmake_properties
from ros2bzl.scraping.ament_python \
    import collect_ament_python_package_direct_properties
from ros2bzl.scraping.ament_python import PackageNotFoundError

from ros2bzl.templates import configure_executable_imports
from ros2bzl.templates import configure_package_c_library_alias
from ros2bzl.templates import configure_package_cc_library
from ros2bzl.templates import configure_package_executable_imports
from ros2bzl.templates import configure_package_interfaces_filegroup
from ros2bzl.templates import configure_package_meta_py_library
from ros2bzl.templates import configure_package_py_library
from ros2bzl.templates import configure_package_share_filegroup
from ros2bzl.templates import configure_prologue

from ros2bzl.utilities import interpolate


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
        '-s', '--sandbox-mapping',
        action='append', dest='sandbox_mappings',
        metavar='OUTER_PATH:INNER_PATH', default=[],
        help='Path mappings for workspace sandboxing'
    )
    parser.add_argument(
        '-j', '--jobs', metavar='N', type=int, default=None,
        help='Number of CMake jobs to use during package configuration scraping'
    )
    parser.add_argument(
        '-d', '--distro-file',
        type=argparse.FileType('r'), required=True,
        help='Path to distro metadata file, in JSON format'
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


def write_build_file(fd, repo_name, distro, sandbox, cache):
    rmw_implementation_packages = {
        name: metadata for name, metadata in distro['packages'].items()
        if 'rmw_implementation_packages' in metadata.get('groups', [])
    }

    template, config = configure_prologue(repo_name)
    fd.write(interpolate(template, config) + '\n')

    dependency_graph = distro['dependency_graph']
    dependency_graph = {k: set(v) for k, v in dependency_graph.items()}
    for name in toposort.toposort_flatten(dependency_graph):
        metadata = distro['packages'][name]

        # Avoid linking non-direct dependencies (e.g. group dependencies)
        direct_dependency_names = \
            dependency_graph[name].intersection(
                set(metadata.get('build_export_dependencies', [])).union(
                    set(metadata.get('run_dependencies', []))
                )
            )
        direct_dependencies = {
            dependency_name: distro['packages'][dependency_name]
            for dependency_name in sorted(direct_dependency_names)
        }

        if 'share_directory' in metadata:
            _, template, config = \
                configure_package_share_filegroup(name, metadata, sandbox)
            fd.write(interpolate(template, config) + '\n')

        if 'rosidl_interface_packages' in metadata.get('groups', []):
            _, template, config = \
                configure_package_interfaces_filegroup(name, metadata, sandbox)
            fd.write(interpolate(template, config) + '\n')

        if 'cmake' in metadata.get('build_type'):
            properties = collect_ament_cmake_package_direct_properties(
                name, metadata, direct_dependencies, cache
            )

            _, template, config = configure_package_cc_library(
                name, metadata, properties, direct_dependencies, sandbox
            )

            fd.write(interpolate(template, config) + '\n')

            if 'rosidl_interface_packages' in metadata.get('groups', []):
                # Alias C++ library as C library for interface packages
                # as their headers and artifacts cannot be discriminated.
                _, template, config = \
                    configure_package_c_library_alias(name, metadata)
                fd.write(interpolate(template, config) + '\n')

        # No way to tell if there's Python code for this package
        # but to look for it.
        try:
            properties = collect_ament_python_package_direct_properties(
                name, metadata, direct_dependencies, cache
            )
            # Add 'py' as language if not there.
            if 'langs' not in metadata:
                metadata['langs'] = []
            if 'py' not in metadata['langs']:
                metadata['langs'].append('py')
        except PackageNotFoundError:
            for dependency_metadata in direct_dependencies.values():
                if 'langs' not in dependency_metadata:
                    continue
                if 'py' not in dependency_metadata['langs']:
                    continue
                # Dependencies still need to be propagated.
                if 'py (transitive)' not in metadata['langs']:
                    metadata['langs'].append('py (transitive)')
                _, template, config = configure_package_meta_py_library(
                    name, metadata, direct_dependencies)
                fd.write(interpolate(template, config) + '\n')
                break
            properties = {}

        if properties:
            _, template, config = configure_package_py_library(
                name, metadata, properties, direct_dependencies, sandbox
            )
            fd.write(interpolate(template, config) + '\n')

        if 'executables' in metadata:
            direct_dependencies.update(rmw_implementation_packages)
            for _, template, config in configure_package_executable_imports(
                name, metadata, direct_dependencies, sandbox
            ):
                fd.write(interpolate(template, config) + '\n')

    for _, template, config in configure_executable_imports(
        distro['executables'], distro['packages'], sandbox
    ):
        fd.write(interpolate(template, config) + '\n')


def main():
    args = parse_arguments()

    ament_cmake_cache = \
        precache_ament_cmake_properties(
            args.distro['packages'], jobs=args.jobs)

    write_build_file(
        args.output, args.repository_name, args.distro,
        args.sandbox, {'ament_cmake': ament_cmake_cache})


if __name__ == '__main__':
    main()
