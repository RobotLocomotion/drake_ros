import glob
from importlib.metadata import distribution
from importlib.metadata import PackageNotFoundError
import sysconfig

from ros2bzl.scraping.system import find_library_dependencies
from ros2bzl.scraping.system import is_system_library

EXTENSION_SUFFIX = sysconfig.get_config_var('EXT_SUFFIX')


def find_python_package(name):
    dist = distribution(name)
    top_level = dist.read_text('top_level.txt')
    top_level = top_level.rstrip('\n')
    return str(dist._path), str(dist.locate_file(top_level))


def collect_ament_python_package_properties(name, metadata):
    egg_path, top_level = find_python_package(name)
    properties = {'python_packages': [(egg_path, top_level)]}
    cc_libraries = glob.glob('{}/**/*.so'.format(top_level),  recursive=True)
    if cc_libraries:
        cc_libraries.extend(set(
            dep for library in cc_libraries
            for dep in find_library_dependencies(library)
            if not is_system_library(dep)
        ))
        properties['cc_extensions'] = [
            lib for lib in cc_libraries
            if lib.endswith(EXTENSION_SUFFIX)
        ]
        properties['cc_libraries'] = [
            lib for lib in cc_libraries
            if not lib.endswith(EXTENSION_SUFFIX)
        ]
    return properties


def collect_ament_python_package_direct_properties(
    name, metadata, dependencies, cache
):
    if 'ament_python' not in cache:
        cache['ament_python'] = {}
    ament_python_cache = cache['ament_python']

    if name not in ament_python_cache:
        ament_python_cache[name] = \
            collect_ament_python_package_properties(name, metadata)

    properties = dict(ament_python_cache[name])

    if 'cc_libraries' in properties:
        ament_cmake_cache = cache['ament_cmake']
        for dependency_name, dependency_metadata in dependencies.items():
            dependency_libraries = []
            if 'cc' in dependency_metadata.get('langs', []):
                if dependency_metadata.get('build_type') == 'ament_cmake':
                    if dependency_name not in ament_cmake_cache:
                        ament_cmake_cache[dependency_name] = \
                            collect_ament_cmake_package_properties(
                                dependency_name, dependency_metadata)
                    dependency_properties = ament_cmake_cache[dependency_name]
                    dependency_libraries.extend(
                        dependency_properties['link_libraries']
                    )
            if 'py' in dependency_metadata.get('langs', []):
                if dependency_name not in ament_python_cache:
                    ament_python_cache[dependency_name] = \
                        collect_ament_python_package_properties(
                            dependency_name, dependency_metadata)
                dependency_properties = ament_python_cache[dependency_name]
                if 'cc_libraries' in dependency_properties:
                    dependency_libraries.extend(
                        dependency_properties['cc_libraries'])
            # Remove duplicates maintaining order
            properties['cc_libraries'] = [
                lib for lib in properties['cc_libraries']
                if lib not in dependency_libraries
            ]
    return properties
