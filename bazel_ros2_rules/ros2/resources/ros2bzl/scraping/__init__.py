import os

import ament_index_python
import cmake_tools

from ros2bzl.scraping.metadata import collect_cmake_package_metadata
from ros2bzl.scraping.metadata import collect_ros_package_metadata


def list_all_executables():
    executables = {}
    for prefix in ament_index_python.get_packages_with_prefixes().values():
        bindir = os.path.join(prefix, 'bin')
        if not os.path.isdir(bindir):
            continue
        for path in os.listdir(bindir):
            path = os.path.join(bindir, path)
            if os.path.isfile(path) and os.access(path, os.X_OK):
                name = os.path.basename(path)
                if name not in executables:
                    executables[name] = path
    return list(executables.values())


def index_all_packages():
    packages = {
        name: collect_ros_package_metadata(name, prefix)
        for name, prefix in
        ament_index_python.get_packages_with_prefixes().items()
    }
    for name, prefix in cmake_tools.get_packages_with_prefixes().items():
        if name in packages:
            # Assume unique package names across package types
            continue
        packages[name] = collect_cmake_package_metadata(name, prefix)
    return packages


def build_dependency_graph(packages, include=None, exclude=None):
    package_set = set(packages)
    if include:
        include = set(include)
        if not package_set.issuperset(include):
            unknown_packages = tuple(include.difference(package_set))
            msg = 'Cannont find package'
            if len(unknown_packages) == 1:
                msg +=  ' ' + repr(unknown_packages[0])
            else:
                msg += 's ' + repr(unknown_packages)
            raise RuntimeError(msg)
        package_set &= include
    if exclude:
        package_set -= exclude

    groups = {}
    for name, metadata in packages.items():
        if 'groups' not in metadata:
            continue
        for group_name in metadata['groups']:
            groups.setdefault(group_name, [])
            groups[group_name].append(name)

    dependency_graph = {}
    while package_set:
        name = package_set.pop()
        metadata = packages[name]
        dependencies = set(metadata.get('build_export_dependencies', []))
        dependencies.update(metadata.get('run_dependencies', []))
        if 'group_dependencies' in metadata:
            for group_name in metadata['group_dependencies']:
                dependencies.update(groups[group_name])
        if exclude:
            dependencies -= exclude
        # Ignore system, non-ROS dependencies
        # NOTE(hidmic): shall we sandbox those too?
        dependencies = {
            dependency_name
            for dependency_name in dependencies
            if dependency_name in packages
        }
        dependency_graph[name] = dependencies
        package_set.update(dependencies)

    packages = {name: packages[name] for name in dependency_graph}

    return packages, dependency_graph


def scrape_distribution(include=None, exclude=None):
    packages, dependency_graph = build_dependency_graph(
        index_all_packages(), include, exclude)
    executables = list_all_executables()
    ld_library_path = os.environ['LD_LIBRARY_PATH']
    return {
        'packages': packages,
        'dependency_graph': dependency_graph,
        'executables': executables,
        'paths': {
            'ament_prefix': ament_index_python.get_search_paths(),
            'library_load': ld_library_path.split(os.path.pathsep),
        }
    }
