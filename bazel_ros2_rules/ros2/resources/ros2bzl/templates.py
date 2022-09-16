import os
import pathlib

from ros2bzl.resources import load_resource
from ros2bzl.scraping.system import find_library_path
from ros2bzl.utilities import to_starlark_string_dict


def label_name(name, metadata):
    return metadata.get('bazel_name', name)


def label(name, metadata):
    workspace = metadata.get('bazel_workspace', '')
    if workspace:
        workspace = '@' + workspace
    package = metadata.get('bazel_package', '')
    if workspace or package:
        package = '//' + package
    return workspace + package + ':' + label_name(name, metadata)


def labels_with(suffix):
    return (
        (lambda *args, **kwargs: label_name(*args, **kwargs) + suffix),
        (lambda *args, **kwargs: label(*args, **kwargs) + suffix)
    )


share_name, share_label = labels_with(suffix='_share')
c_name, c_label = labels_with(suffix='_c')
cc_name, cc_label = labels_with(suffix='_cc')
py_name, py_label = labels_with(suffix='_py')
meta_py_name, meta_py_label = labels_with(suffix='_transitive_py')


def configure_package_share_filegroup(name, metadata, sandbox):
    target_name = share_name(name, metadata)
    shared_directories = [sandbox(metadata['share_directory'])]
    if 'ament_index_directory' in metadata:
        shared_directories.append(sandbox(metadata['ament_index_directory']))
    return (
        target_name,
        load_resource('templates/package_share_filegroup.bazel.tpl'),
        to_starlark_string_dict({
            'name': target_name, 'share_directories': shared_directories
        })
    )


def configure_package_interfaces_filegroup(name, metadata, sandbox):
    return (
        name,
        load_resource('templates/package_interfaces_filegroup.bazel.tpl'),
        to_starlark_string_dict({
            'name': name,
            'share_directory': sandbox(metadata['share_directory'])
        })
    )


def configure_package_cc_library(
    name, metadata, properties, dependencies, sandbox
):
    target_name = cc_name(name, metadata)
    libraries = [sandbox(library) for library in properties['link_libraries']]
    include_directories = [
        sandbox(include) for include in properties['include_directories']]
    local_includes = [
        include for include in include_directories
        if not os.path.isabs(include)]
    headers = []
    for include in local_includes:
        if name not in pathlib.Path(include).parts:
            # Assume package lives in a merged install space
            # Assume package abides to REP-122 FHS layout
            include = os.path.join(include, name)
        headers.append(include)
    # Push remaining nonlocal includes through compiler options
    copts = [
        '-isystem ' + include
        for include in include_directories
        if os.path.isabs(include)]
    copts.extend(properties['compile_flags'])
    defines = properties['defines']

    linkopts = properties['link_flags']
    for link_directory in properties['link_directories']:
        link_directory = sandbox(link_directory)
        if not link_directory:
            continue
        linkopts += [
            '-L' + link_directory,
            '-Wl,-rpath ' + link_directory
        ]
    deps = [
        cc_label(dependency_name, dependency_metadata)
        for dependency_name, dependency_metadata in dependencies.items()
        if 'cc' in dependency_metadata.get('langs', [])
    ]

    data = []
    if 'share_directory' in metadata:
        data.append(share_label(name, metadata))
    # Add in plugins, if any
    if 'plugin_libraries' in metadata:
        data.extend(
            sandbox(find_library_path(library))
            for library in metadata['plugin_libraries']
        )
    # Prepare runfiles to support dynamic loading
    data.extend(library for library in libraries if library not in data)

    template_path = 'templates/package_cc_library.bazel.tpl'

    config = {
        'name': target_name,
        'srcs': libraries,
        'headers': headers,
        'includes': local_includes,
        'copts': copts,
        'defines': defines,
        'linkopts': linkopts,
        'data': data,
        'deps': deps,
    }

    return (
        target_name, load_resource(template_path),
        to_starlark_string_dict(config))


def configure_package_meta_py_library(name, metadata, dependencies):
    deps = []
    for dependency_name, dependency_metadata in dependencies.items():
        if 'py' in dependency_metadata.get('langs', []):
            deps.append(py_label(dependency_name, dependency_metadata))
        elif 'py (transitive)' in dependency_metadata.get('langs', []):
            deps.append(meta_py_label(dependency_name, dependency_metadata))
    target_name = meta_py_name(name, metadata)
    return (
        target_name,
        load_resource('templates/package_meta_py_library.bazel.tpl'),
        to_starlark_string_dict({'name': target_name, 'deps': deps})
    )


def configure_package_py_library(
    name, metadata, properties, dependencies, sandbox
):
    target_name = py_name(name, metadata)
    eggs = [sandbox(egg_path) for egg_path, _ in properties['python_packages']]
    tops = [
        sandbox(top_level) for _, top_level in properties['python_packages']]
    imports = [os.path.dirname(egg) for egg in eggs]

    template = 'templates/package_py_library.bazel.tpl'
    config = {
        'name': target_name,
        'tops': tops,
        'eggs': eggs,
        'imports': imports
    }

    deps = []
    for dependency_name, dependency_metadata in dependencies.items():
        if 'py' in dependency_metadata.get('langs', []):
            deps.append(py_label(dependency_name, dependency_metadata))
        elif 'py (transitive)' in dependency_metadata.get('langs', []):
            deps.append(meta_py_label(dependency_name, dependency_metadata))
    config['deps'] = deps

    data = [share_label(name, metadata)]
    if 'cc' in metadata.get('langs', []):
        data.append(cc_label(name, metadata))

    if 'cc_extensions' in properties:
        # Bring in C/C++ dependencies
        cc_deps = [
            cc_label(dependency_name, dependency_metadata)
            for dependency_name, dependency_metadata in dependencies.items()
            if 'cc' in dependency_metadata.get('langs', [])
        ]
        # Expose C/C++ libraries if any
        if 'cc_libraries' in properties:
            template = 'templates/package_py_library_with_cc_libs.bazel.tpl'
            config.update({
                'cc_name': c_name("_" + target_name, metadata),
                'cc_libs': [
                    sandbox(lib) for lib in properties['cc_libraries']],
                'cc_deps': cc_deps
            })
            data.append(c_label("_" + target_name, metadata))
        else:
            data.extend(cc_deps)
        # Prepare runfiles to support dynamic loading
        cc_extensions = [
            sandbox(extension) for extension in properties['cc_extensions']]
        data.extend(cc_extensions)

    # Add in plugins, if any
    if 'plugin_libraries' in metadata:
        data.extend(
            sandbox(find_library_path(library))
            for library in metadata['plugin_libraries']
        )

    config['data'] = data

    return (
        target_name,
        load_resource(template),
        to_starlark_string_dict(config)
    )


def configure_package_alias(name, target):
    return (
        name,
        load_resource('templates/package_alias.bazel.tpl'),
        to_starlark_string_dict({'name': name, 'actual': ':' + target})
    )


def configure_package_c_library_alias(name, metadata):
    target_name = c_name(name, metadata)
    return (
        target_name,
        load_resource('templates/package_alias.bazel.tpl'),
        to_starlark_string_dict({
            'name': target_name,
            'actual': cc_label(name, metadata)
        })
    )


def configure_executable_imports(
    executables, dependencies, sandbox, prefix=None
):
    deps = []
    common_data = []
    for dependency_name, dependency_metadata in dependencies.items():
        # TODO(hidmic): use appropriate target based on executable file type
        if 'cc' in dependency_metadata.get('langs', []):
            common_data.append(cc_label(dependency_name, dependency_metadata))
        if 'py' in dependency_metadata.get('langs', []):
            deps.append(py_label(dependency_name, dependency_metadata))
        elif 'py (transitive)' in dependency_metadata.get('langs', []):
            common_data.append(meta_py_label(
                dependency_name, dependency_metadata))

    for executable in executables:
        target_name = os.path.basename(executable)
        if prefix:
            target_name = prefix + '_' + target_name
        yield (
            target_name,
            load_resource('templates/overlay_executable.bazel.tpl'),
            to_starlark_string_dict({
                'name': target_name,
                'executable': sandbox(executable),
                'data': common_data, 'deps': deps,
            })
        )


def configure_package_executable_imports(
    name, metadata, dependencies, sandbox
):
    dependencies = dict(dependencies)
    dependencies[name] = metadata
    yield from configure_executable_imports(
        metadata['executables'], dependencies,
        sandbox, prefix=label_name(name, metadata)
    )


def configure_prologue(repo_name):
    return load_resource('templates/prologue.bazel'), {}
