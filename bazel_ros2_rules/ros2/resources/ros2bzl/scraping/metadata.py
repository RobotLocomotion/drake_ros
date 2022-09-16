import os
import xml.etree.ElementTree as ET


def parse_package_xml(path_to_package_xml):
    tree = ET.parse(path_to_package_xml)

    depends = set([
        tag.text for tag in tree.findall('./depend')
    ])
    exec_depends = set([
        tag.text for tag in tree.findall('./exec_depend')
    ])
    build_export_depends = set([
        tag.text for tag in tree.findall('./build_export_depend')
    ])
    group_depends = set([
        tag.text for tag in tree.findall('./group_depend')
    ])
    member_of_groups = set([
        tag.text for tag in tree.findall('./member_of_group')
    ])
    build_type = tree.find('./export/build_type').text

    return dict(
        build_export_dependencies=build_export_depends | depends,
        run_dependencies=exec_depends | depends,
        group_dependencies=group_depends,
        groups=member_of_groups,
        build_type=build_type
    )


def parse_plugins_description_xml(path_to_plugins_description_xml):
    plugins_description_xml = ET.parse(path_to_plugins_description_xml)
    root = plugins_description_xml.getroot()
    assert root.tag == 'library'
    return dict(plugin_libraries=[root.attrib['path']])


def find_executables(base_path):
    for dirpath, dirnames, filenames in os.walk(base_path):
        # ignore folder starting with .
        dirnames[:] = [d for d in dirnames if d[0] not in ['.']]
        dirnames.sort()
        # select executable files
        for filename in sorted(filenames):
            path = os.path.join(dirpath, filename)
            if os.access(path, os.X_OK):
                yield path


DEFAULT_LANGS_PER_BUILD_TYPE = {
    'cmake': {'cc'},
    'ament_cmake': {'cc'},
    'ament_python': {'py'},
}


def collect_package_langs(metadata):
    build_type = metadata.get('build_type')
    if build_type in DEFAULT_LANGS_PER_BUILD_TYPE:
        return set(DEFAULT_LANGS_PER_BUILD_TYPE[build_type])
    return set()


def collect_cmake_package_metadata(name, prefix):
    metadata = dict(prefix=prefix, build_type='cmake')
    metadata['langs'] = collect_package_langs(metadata)
    return metadata


def collect_ros_package_metadata(name, prefix):
    """
    Collects ROS package metadata.

    Metadata includes package `prefix`, `share_directory`,
    `ament_index_directory`, `build_type`, `build_export_dependencies`,
    `run_dependencies`, `group_dependencies`, `groups`, `plugin_libraries`,
    `executables`, and `langs` (i.e expected code languages).

    This function supports both symlink and merged install workspaces.

    :param name: ROS package name
    :param prefix: ROS package install prefix
    :returns: metadata as a dictionary
    """
    share_directory = os.path.join(prefix, 'share', name)
    ament_index_directory = os.path.join(prefix, 'share', 'ament_index')
    resource_index_directory = os.path.join(
        ament_index_directory, 'resource_index')

    metadata = dict(
        prefix=prefix,
        share_directory=share_directory,
        ament_index_directory=ament_index_directory,
    )

    lib_directory = os.path.join(prefix, 'lib', name)
    executables = list(find_executables(lib_directory))
    if executables:
        metadata['executables'] = executables

    path_to_package_xml = os.path.join(share_directory, 'package.xml')
    metadata.update(parse_package_xml(path_to_package_xml))

    metadata['langs'] = collect_package_langs(metadata)

    # Find any plugins provided by this package
    plugin_libraries = []
    for resource_type in os.listdir(resource_index_directory):
        # Pluginlib adds this suffix to the ament index resource type
        # https://github.com/ros/pluginlib/blob/
        # a11ecea28a587637d51f036e0e04eb194b94abfe/pluginlib/cmake/
        # pluginlib_package_hook.cmake#L22
        if resource_type.endswith('__pluginlib__plugin'):
            plugin_resource = os.path.join(
                resource_index_directory, resource_type, name)
            if os.path.exists(plugin_resource):
                with open(plugin_resource, 'r') as fin:
                    path_to_desc = fin.read()
                # Strip any trailing newline
                path_to_desc = path_to_desc.strip()
                if not os.path.isabs(path_to_desc):
                    path_to_desc = os.path.join(prefix, path_to_desc)
                if os.path.exists(path_to_desc):
                    plugin_libraries.extend(parse_plugins_description_xml(
                        path_to_desc)['plugin_libraries'])
    if plugin_libraries:
        metadata['plugin_libraries'] = plugin_libraries

    return metadata
