# -*- python -*-

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@python_dev//:version.bzl", "PYTHON_EXTENSION_SUFFIX")
load("//tools:ament_index.bzl", "AmentIndex")
load(
    ":distro.bzl",
    "AVAILABLE_TYPESUPPORT_LIST",
    "REPOSITORY_ROOT"
)
load(
    ":_calculate_rosidl_capitalization.bzl",
    "calculate_rosidl_capitalization"
)

RosInterfaces = provider(
    fields = ["interfaces"],
)

def _as_idl_tuple(file):
    """
    Returns IDL tuple for a file as expected by `rosidl` commands.
    """
    path, parent, base = file.path.rsplit("/", 2)
    if parent not in ("msg", "srv", "action"):
        fail("Interface parent folder must be one of: 'msg', 'srv', 'action'")
    return "{}:{}/{}".format(path, parent, base)

def _as_include_flag(file):
    """
    Returns path to file as an include flag for `rosidl` commands.
    """
    return "-I" + file.path.rsplit("/", 3)[0]

def _rosidl_generate_genrule_impl(ctx):
    args = ctx.actions.args()
    args.add("generate")
    output_path_parts = [
        ctx.var["GENDIR"], ctx.label.workspace_root,
        ctx.label.package, ctx.attr.output_dir]
    output_path = "/".join([
        part for part in output_path_parts if part])
    args.add("--output-path", output_path)
    for type in ctx.attr.types:
        args.add("--type", type)
    for typesupport in ctx.attr.typesupports:
        args.add("--type-support", typesupport)
    args.add_all(ctx.files.includes, map_each=_as_include_flag, uniquify=True)
    args.add(ctx.attr.group)
    args.add_all(ctx.files.interfaces, map_each=_as_idl_tuple)
    inputs = ctx.files.interfaces + ctx.files.includes
    ctx.actions.run_shell(
        tools = [ctx.executable._tool],
        arguments = [args], inputs = inputs,
        command = "{} $@ > /dev/null".format(
            ctx.executable._tool.path
        ),
        outputs = ctx.outputs.generated_sources,
    )

rosidl_generate_genrule = rule(
    attrs = dict(
        generated_sources = attr.output_list(mandatory = True),
        types = attr.string_list(mandatory = False),
        typesupports = attr.string_list(mandatory = False),
        group = attr.string(mandatory = True),
        interfaces = attr.label_list(
            mandatory = True, allow_files = True
        ),
        includes = attr.label_list(mandatory = False),
        output_dir = attr.string(mandatory = False),
        _tool = attr.label(
            default = REPOSITORY_ROOT + ":rosidl",
            executable = True, cfg = "exec"
        )
    ),
    implementation = _rosidl_generate_genrule_impl,
    output_to_genfiles = True,
)
"""
Generates ROS 2 interface type representation and/or type support sources.

Args:
    generated_sources: expected sources after generation.
    types: list of type representations to generate (e.g. cpp, py).
    typesupports: list of type supports to generate (e.g. typesupport_cpp).
    group: interface group name (i.e. ROS 2 package name).
    interfaces: interface definition files, both files and filegroups are allowed.
    includes: optional interface definition includes, both files and filegroups are allowed.
    output_dir: optional output subdirectory.

See `rosidl generate` CLI for further reference.
"""

def _rosidl_translate_genrule_impl(ctx):
    args = ctx.actions.args()
    args.add("translate")
    output_path_parts = [
        ctx.var["GENDIR"], ctx.label.workspace_root,
        ctx.label.package, ctx.attr.output_dir]
    output_path = "/".join([
        part for part in output_path_parts if part])
    args.add("--output-path", output_path)
    args.add("--output-format", ctx.attr.output_format)
    if ctx.attr.input_format:
        args.add("--input-format", ctx.attr.input_format)
    args.add_all(ctx.files.includes, map_each=_as_include_flag, uniquify=True)
    args.add(ctx.attr.group)
    args.add_all(ctx.files.interfaces, map_each=_as_idl_tuple)
    inputs = ctx.files.interfaces + ctx.files.includes
    ctx.actions.run_shell(
        tools = [ctx.executable._tool],
        arguments = [args], inputs = inputs,
        command = "{} $@ > /dev/null".format(
            ctx.executable._tool.path
        ),
        outputs = ctx.outputs.translated_interfaces
    )
    return [RosInterfaces(interfaces = ctx.outputs.translated_interfaces)]

rosidl_translate_genrule = rule(
    attrs = dict(
        translated_interfaces = attr.output_list(mandatory = True),
        output_format = attr.string(mandatory = True),
        input_format = attr.string(mandatory = False),
        group = attr.string(mandatory = True),
        interfaces = attr.label_list(
            mandatory = True, allow_files = True
        ),
        includes = attr.label_list(mandatory = False),
        output_dir = attr.string(mandatory = False),
        _tool = attr.label(
            default = REPOSITORY_ROOT + ":rosidl",
            executable = True, cfg = "exec"
        )
    ),
    implementation = _rosidl_translate_genrule_impl,
    output_to_genfiles = True,
)
"""
Translates ROS 2 interface definition files.

Args:
    translated_interfaces: execpted interface definition files after translation.
    output_format: output format to translate interface definition files to.
    input_format: optional input format, deduced from file extensions by default.
    group: interface group name (i.e. ROS 2 package name).
    interfaces: interface definition files, both files and filegroups are allowed.
    includes: optional interface definition includes, both files and filegroups are allowed.
    output_dir: optional output subdirectory.

See `rosidl translate` CLI for further reference.
"""

def _rosidl_generate_ament_index_entry_impl(ctx):
    # declare that a "package" with the group name exists
    package_marker_path = paths.join(
        ctx.attr.prefix,
        'share/ament_index/resource_index/packages/',
        ctx.attr.group,
    )
    package_marker_out = ctx.actions.declare_file(package_marker_path)
    ctx.actions.write(
        output = package_marker_out,
        content = ""
    )

    # Declare interface files in a file under the rosidl_interfaces resource.
    # Directory names are important.
    # https://github.com/ament/ament_cmake/blob/1.5.0/ament_cmake_core/
    # doc/resource_index.md#file-system-index-layout
    manifest_path = paths.join(
        ctx.attr.prefix,
        'share/ament_index/resource_index/rosidl_interfaces/',
        ctx.attr.group,
    )
    manifest_out = ctx.actions.declare_file(manifest_path)

    # Gather interface files from arguments which may be rules or files.
    interface_files = []
    for input_file_or_rule in ctx.attr.interfaces:
        if RosInterfaces in input_file_or_rule:
            rule = input_file_or_rule
            for file in rule[RosInterfaces].interfaces:
                interface_files.append(file)
        else:
            input_file = input_file_or_rule
            for file in input_file.files.to_list():
                interface_files.append(file)

    # Reduce names to "{msg|srv|action}/{message name}.{msg|srv|action|idl}
    rosidl_interfaces_manifest = []
    for file in interface_files:
        # file.short_path is:
        # {some path}/{msg|srv|action}/{message name}.{msg|srv|action|idl}
        file_name = file.short_path.split("/")[-1]
        interface_type = file.short_path.split("/")[-2]
        rosidl_interfaces_manifest.append(interface_type + "/" + file_name)

    ctx.actions.write(
        output = manifest_out,
        content = "\n".join(rosidl_interfaces_manifest)
    )

    # Symlink interface files into the share directory
    runfiles_symlinks = {
      package_marker_path: package_marker_out,
      manifest_path: manifest_out
    }
    for path, short_path in zip(interface_files, rosidl_interfaces_manifest):
        symlink_path = paths.join(
            ctx.attr.prefix, "share", ctx.attr.group, short_path)
        runfiles_symlinks[symlink_path] = path

    return [
        AmentIndex(prefix = ctx.attr.prefix),
        DefaultInfo(
            runfiles = ctx.runfiles(root_symlinks = runfiles_symlinks)
        ),
    ]

rosidl_generate_ament_index_entry = rule(
    attrs = dict(
        group = attr.string(mandatory = True),
        interfaces = attr.label_list(
            mandatory = True, allow_empty = False, allow_files = True),
        prefix = attr.string(default = "."),
    ),
    implementation = _rosidl_generate_ament_index_entry_impl,
    output_to_genfiles = True,
    provides = [AmentIndex],
)

def _deduce_source_parts(interface_path):
    """
    Deduces source subdirectory and basename for a given path to an interface definition file.
    """
    parent, _, base = interface_path.rpartition("/")
    basename, _, ext = base.rpartition(".")

    return parent, calculate_rosidl_capitalization(basename)

def rosidl_definitions_filegroup(name, group, interfaces, includes, **kwargs):
    """
    Generates ROS 2 interfaces .idl definitions.

    This rule standardizes all interface definitions' format to IDL.

    Args:
        name: filegroup target name.
        group: interface group name (i.e. ROS 2 package name).
        interfaces: interface definition files, both files and filegroups are allowed
        includes: optional interface definition includes, both files and filegroups are allowed

    Additional keyword arguments are those common to all rules.
    """
    translated_interfaces = []
    for ifc in interfaces:
        base, _, ext = ifc.rpartition(".")
        translated_interfaces.append(base + ".idl")
    rosidl_translate_genrule(
        name = name,
        output_format = "idl",
        translated_interfaces = translated_interfaces,
        group = group,
        interfaces = interfaces,
        includes = includes,
        **kwargs
    )

def _deduce_source_paths(group, kind):
    """
    Deduces include and root paths for generated sources of a given group and kind.
    """
    include = "{}/{}".format(group, kind)
    root = "{}/{}".format(include, group)
    return include, root

def _make_public_name(name, suffix=""):
    """
    Builds a public name (i.e. with no leading underscore) from a
    private or public name.
    """
    return name.lstrip("_") + suffix

def _make_private_name(name, suffix=""):
    """
    Builds a private name (i.e. with leading underscore) from a
    private or public name.
    """
    if not name.startswith("_"):
        name = "_" + name
    return name + suffix

def _make_public_label(label, suffix=""):
    """
    Builds a public label (i.e. name with no leading underscore)
    from a private or public label, or a plain name (assumed to
    be local to the calling package).
    """
    package, _, name = label.rpartition(":")
    prefix, _, name = name.rpartition("/")
    if prefix:
        prefix = prefix + "/"
    return package + ":" + prefix + _make_public_name(name, suffix)

def _make_private_label(label, suffix=""):
    """
    Builds a private label (i.e. name with leading underscore)
    from a private or public label, or a plain name (assumed to
    be local to the calling package).
    """
    package, _, name = label.rpartition(":")
    prefix, _, name = name.rpartition("/")
    if prefix:
        prefix = prefix + "/"
    return package + ":" + prefix + _make_private_name(name, suffix)

def rosidl_c_library(
    name, group, interfaces, includes = [], deps = [],
    cc_library_rule = native.cc_library, **kwargs
):
    """
    Generates and builds C ROS 2 interfaces.

    Args:
        name: C library target name.
        group: interface group name (i.e. ROS 2 package name).
        interfaces: interface definition files, only files are allowed
        includes: optional interface definition includes, both files and filegroups are allowed.
        deps: optional library dependencies.
        cc_library_rule: optional cc_library() rule override.

    Additional keyword arguments are those common to all rules.
    """
    include, root = _deduce_source_paths(group, "c")

    generated_c_sources = []
    visibility_header = "msg/rosidl_generator_c__visibility_control.h"
    generated_c_headers = ["{}/{}".format(root, visibility_header)]
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        generated_c_headers.append("{}/{}/{}.h".format(root, parent, basename))
        generated_c_headers.append("{}/{}/detail/{}__functions.h".format(root, parent, basename))
        generated_c_headers.append("{}/{}/detail/{}__struct.h".format(root, parent, basename))
        generated_c_headers.append("{}/{}/detail/{}__type_support.h".format(root, parent, basename))
        generated_c_sources.append("{}/{}/detail/{}__functions.c".format(root, parent, basename))
    generated_sources = generated_c_sources + generated_c_headers

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_sources,
        types = ["c"],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    deps = deps + [
        REPOSITORY_ROOT + ":rcutils_cc",
        REPOSITORY_ROOT + ":rosidl_runtime_c_cc",
        REPOSITORY_ROOT + ":rosidl_typesupport_interface_cc",
    ]

    cc_library_rule(
        name = name,
        srcs = generated_c_sources,
        hdrs = generated_c_headers,
        includes = [include],
        deps = deps,
        **kwargs
    )

def rosidl_cc_library(
    name, group, interfaces, includes = [], deps = [],
    cc_library_rule = native.cc_library, **kwargs
):
    """
    Generates and builds C++ ROS 2 interfaces.

    Args:
        name: C++ library target name.
        group: interface group name (i.e. ROS 2 package name).
        interfaces: interface definition files.
        includes: optional interface definition includes.
        deps: optional library dependencies.
        cc_library_rule: optional cc_library() rule override.

    Additional keyword arguments are those common to all rules.
    """
    include, root = _deduce_source_paths(group, "cpp")

    generated_cc_headers = []
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        generated_cc_headers.append("{}/{}/{}.hpp".format(root, parent, basename))
        generated_cc_headers.append("{}/{}/detail/{}__builder.hpp".format(root, parent, basename))
        generated_cc_headers.append("{}/{}/detail/{}__struct.hpp".format(root, parent, basename))
        generated_cc_headers.append("{}/{}/detail/{}__traits.hpp".format(root, parent, basename))

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_cc_headers,
        types = ["cpp"],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    cc_library_rule(
        name = name,
        hdrs = generated_cc_headers,
        includes = [include],
        deps = deps + [
            REPOSITORY_ROOT + ":rosidl_runtime_cpp_cc"
        ],
        **kwargs
    )

def _make_c_typesupport_extension_name(group, typesupport_name):
    return "{}_s__{}".format(group, typesupport_name) + PYTHON_EXTENSION_SUFFIX

def rosidl_py_library(
    name, group, interfaces, typesupports,
    includes = [], c_deps = [], py_deps = [],
    cc_binary_rule = native.cc_binary,
    cc_library_rule = native.cc_library,
    py_library_rule = native.py_library,
    **kwargs
):
    """
    Generates and builds Python ROS 2 interfaces, including any C extensions.

    Args:
        name: Python library target name.
        group: interface group name (i.e. ROS 2 package name).
        interfaces: interface definition files.
        typesupports: a mapping of available, vendor-specific
            C typesupport libraries, from typesupport name to
            library target label.
        includes: optional interface definition includes.
        c_deps: optional Python C extension dependencies.
        py_deps: optional Python dependencies.
        cc_binary_rule: optional cc_binary() rule override.
        cc_library_rule: optional cc_library() rule override.
        py_library_rule: optional py_library() rule override.

    Additional keyword arguments are those common to all rules.
    """
    import_, root = _deduce_source_paths(group, "py")

    generated_c_sources = []
    generated_py_sources = []
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        py_source = "{}/{}/__init__.py".format(root, parent)
        if py_source not in generated_py_sources:
            generated_py_sources.append(py_source)
        generated_py_sources.append(
            "{}/{}/_{}.py".format(root, parent, basename))
        generated_c_sources.append(
            "{}/{}/_{}_s.c".format(root, parent, basename))
    generated_sources = generated_c_sources + generated_py_sources

    generated_c_sources_per_typesupport = {}
    for typesupport_name in typesupports:
        generated_c_sources_per_typesupport[typesupport_name] = [
            "{}/_{}_s.ep.{}.c".format(root, group, typesupport_name)]
        generated_sources += generated_c_sources_per_typesupport[typesupport_name]

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_sources,
        types = [
            "py[typesupport_implementations:[{}]]"
            .format(",".join(typesupports))
        ],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    cc_library_rule(
        name = _make_private_name(name, "_c"),
        srcs = generated_c_sources,
        deps = c_deps + [
            _make_private_label(dep, "_c") for dep in py_deps
        ] + ["@python_dev//:libs"],
        **kwargs
    )

    c_typesupport_extension_deps = c_deps + [
        _make_private_label(name, "_c"),
        REPOSITORY_ROOT + ":rosidl_generator_py_cc",
        REPOSITORY_ROOT + ":rosidl_runtime_c_cc",
        REPOSITORY_ROOT + ":rosidl_typesupport_c_cc",
        REPOSITORY_ROOT + ":rosidl_typesupport_interface_cc",
        "@python_dev//:libs",
    ]
    py_data = []
    for typesupport_name, typesupport_library in typesupports.items():
        deps = list(c_typesupport_extension_deps)
        if typesupport_library not in deps:
            deps.append(typesupport_library)
        c_typesupport_extension = "{}/{}".format(
            root, _make_c_typesupport_extension_name(group, typesupport_name))
        cc_binary_rule(
            name = c_typesupport_extension,
            srcs = generated_c_sources_per_typesupport[typesupport_name],
            deps = deps, linkshared = True, **kwargs
        )
        py_data.append(c_typesupport_extension)

    py_library_rule(
        name = name,
        srcs = generated_py_sources,
        imports = [import_],
        data = py_data,
        deps = py_deps,
        **kwargs
    )

def rosidl_typesupport_fastrtps_cc_library(
    name, group, interfaces, includes = [], deps = [],
    cc_binary_rule = native.cc_binary, cc_library_rule = native.cc_library,
    **kwargs
):
    """
    Generates and builds FastRTPS C++ typesupport for ROS 2 interfaces.

    Args:
        name: FastRTPS C++ typesupport library target name.
        group: interface group name (i.e. ROS 2 package name).
        interfaces: interface definition files.
        includes: optional interface definition includes.
        deps: optional library dependencies.
        cc_binary_rule: optional cc_binary() rule override.
        cc_library_rule: optional cc_library() rule override.

    Additional keyword arguments are those common to all rules.
    """
    include, root = _deduce_source_paths(group, "typesupport/fastrtps_cpp")

    generated_cc_sources = []
    visibility_header = "msg/rosidl_typesupport_fastrtps_cpp__visibility_control.h"
    generated_cc_headers = ["{}/{}".format(root, visibility_header)]
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        template = "{}/{}/detail/dds_fastrtps/{}__type_support.cpp"
        generated_cc_sources.append(template.format(root, parent, basename))
        template = "{}/{}/detail/{}__rosidl_typesupport_fastrtps_cpp.hpp"
        generated_cc_headers.append(template.format(root, parent, basename))
    generated_sources = generated_cc_sources + generated_cc_headers

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_sources,
        typesupports = ["fastrtps_cpp"],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    cc_library_rule(
        name = _make_private_name(name, "_hdrs"),
        hdrs = generated_cc_headers,
        includes = [include],
        linkstatic = True,
        **kwargs
    )

    cc_binary_rule(
        name = name,
        srcs = generated_cc_sources,
        deps = deps + [
            _make_private_label(name, "_hdrs"),
            REPOSITORY_ROOT + ":fastcdr_cc",
            REPOSITORY_ROOT + ":rmw_cc",
            REPOSITORY_ROOT + ":rosidl_runtime_c_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_fastrtps_cpp_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_interface_cc",
        ],
        linkshared = True,
        **kwargs
    )

def rosidl_typesupport_fastrtps_c_library(
    name, group, interfaces, includes = [], deps = [],
    cc_binary_rule = native.cc_binary, cc_library_rule = native.cc_library,
    **kwargs
):
    """
    Generates and builds FastRTPS C typesupport for ROS 2 interfaces.

    Args:
        name: FastRTPS C typesupport library target name.
        group: interface group name (i.e. ROS 2 package name).
        interfaces: interface definition files.
        includes: optional interface definition includes.
        deps: optional library dependencies.
        cc_binary_rule: optional cc_binary() rule override.
        cc_library_rule: optional cc_library() rule override.

    Additional keyword arguments are those common to all rules.
    """
    include, root = _deduce_source_paths(group, "typesupport/fastrtps_c")

    generated_c_sources = []
    visibility_header = "msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
    generated_c_headers = ["{}/{}".format(root, visibility_header)]
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        template = "{}/{}/detail/{}__type_support_c.cpp"
        generated_c_sources.append(template.format(root, parent, basename))
        template = "{}/{}/detail/{}__rosidl_typesupport_fastrtps_c.h"
        generated_c_headers.append(template.format(root, parent, basename))
    generated_sources = generated_c_sources + generated_c_headers

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_sources,
        typesupports = ["fastrtps_c"],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    cc_library_rule(
        name = _make_private_name(name, "_hdrs"),
        hdrs = generated_c_headers,
        includes = [include],
        linkstatic = True,
        **kwargs
    )

    cc_binary_rule(
        name = name,
        srcs = generated_c_sources,
        linkshared = True,
        deps = deps + [
            _make_private_label(name, "_hdrs"),
            REPOSITORY_ROOT + ":fastcdr_cc",
            REPOSITORY_ROOT + ":rmw_cc",
            REPOSITORY_ROOT + ":rosidl_runtime_c_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_fastrtps_c_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_fastrtps_cpp_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_interface_cc",
        ],
        **kwargs
    )

def rosidl_typesupport_introspection_c_library(
    name, group, interfaces, includes = [], deps = [],
    cc_binary_rule = native.cc_binary, cc_library_rule = native.cc_library,
    **kwargs
):
    """
    Generates and builds C introspection typesupport for ROS 2 interfaces.

    Args:
        name: C introspection typesupport library target name.
        group: interface group name (i.e. ROS 2 package name).
        interfaces: interface definition files.
        includes: optional interface definition includes.
        deps: optional library dependencies.
        cc_binary_rule: optional cc_binary() rule override.
        cc_library_rule: optional cc_library() rule override.

    Additional keyword arguments are those common to all rules.
    """
    include, root = _deduce_source_paths(group, "typesupport/introspection_c")

    generated_c_sources = []
    visibility_header = "msg/rosidl_typesupport_introspection_c__visibility_control.h"
    generated_c_headers = ["{}/{}".format(root, visibility_header)]
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        generated_c_sources.append(
            "{}/{}/detail/{}__type_support.c".format(root, parent, basename))
        template = "{}/{}/detail/{}__rosidl_typesupport_introspection_c.h"
        generated_c_headers.append(template.format(root, parent, basename))
    generated_sources = generated_c_sources + generated_c_headers

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_sources,
        typesupports = ["introspection_c"],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    cc_library_rule(
        name = _make_private_name(name, "_hdrs"),
        hdrs = generated_c_headers,
        includes = [include],
        linkstatic = True,
        **kwargs
    )

    cc_binary_rule(
        name = name,
        srcs = generated_c_sources,
        linkshared = True,
        deps = deps + [
            _make_private_label(name, "_hdrs"),
            REPOSITORY_ROOT + ":rosidl_typesupport_introspection_c_cc",
        ],
        **kwargs
    )

def rosidl_typesupport_introspection_cc_library(
    name, group, interfaces, includes = [], deps = [],
    cc_binary_rule = native.cc_binary, cc_library_rule = native.cc_library,
    **kwargs
):
    """
    Generates and builds C++ introspection typesupport for ROS 2 interfaces.

    Args:
        name: C++ introspection typesupport library target name.
        group: interface group name (i.e. ROS 2 package name).
        interfaces: interface definition files.
        includes: optional interface definition includes.
        deps: optional library dependencies.
        cc_binary_rule: optional cc_binary() rule override.
        cc_library_rule: optional cc_library() rule override.

    Additional keyword arguments are those common to all rules.
    """
    include, root = _deduce_source_paths(group, "typesupport/introspection_cpp")

    generated_cc_sources = []
    generated_cc_headers = []
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        generated_cc_sources.append(
            "{}/{}/detail/{}__type_support.cpp".format(root, parent, basename))
        template = "{}/{}/detail/{}__rosidl_typesupport_introspection_cpp.hpp"
        generated_cc_headers.append(template.format(root, parent, basename))
    generated_sources = generated_cc_sources + generated_cc_headers

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_sources,
        typesupports = ["introspection_cpp"],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    cc_library_rule(
        name = _make_private_name(name, "_hdrs"),
        hdrs = generated_cc_headers,
        includes = [include],
        linkstatic = True,
        **kwargs
    )

    cc_binary_rule(
        name = name,
        srcs = generated_cc_sources,
        linkshared = True,
        deps = deps + [
            _make_private_label(name, "_hdrs"),
            REPOSITORY_ROOT + ":rosidl_runtime_c_cc",
            REPOSITORY_ROOT + ":rosidl_runtime_cpp_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_interface_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_introspection_cpp_cc",
        ],
        **kwargs
    )

def rosidl_typesupport_c_library(
    name, group, interfaces, typesupports, includes = [],
    deps = [], cc_binary_rule = native.cc_binary, **kwargs
):
    """
    Generates and builds ROS 2 interfaces' C typesupport.

    Args:
        name: C typesupport library target name
        group: interface group name (i.e. ROS 2 package name)
        interfaces: interface definition files
        typesupports: a mapping of available, vendor-specific
            C typesupport libraries, from typesupport name to
            library target label.
        includes: optional interface definition includes
        deps: optional library dependencies
        cc_binary_rule: optional cc_binary() rule override

    Additional keyword arguments are those common to all rules.
    """
    include, root = _deduce_source_paths(group, "typesupport/c")

    generated_c_sources = []
    generated_c_headers = []
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        generated_c_sources.append(
            "{}/{}/{}__type_support.cpp".format(root, parent, basename))
    generated_sources = generated_c_sources + generated_c_headers

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_sources,
        typesupports = [
            "c[typesupport_implementations:[{}]]"
            .format(",".join(typesupports))
        ],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    cc_binary_rule(
        name = name,
        srcs = generated_sources,
        includes = [include],
        linkshared = True,
        data = typesupports.values(),
        deps = deps + [
            _make_private_label(label, "_hdrs")
            for label in typesupports.values()
        ] + [
            REPOSITORY_ROOT + ":rosidl_runtime_c_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_c_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_interface_cc",
        ],
        **kwargs
    )

def rosidl_typesupport_cc_library(
    name, group, interfaces, typesupports, includes = [],
    deps = [], cc_binary_rule = native.cc_binary, **kwargs
):
    """
    Generates and builds ROS 2 interfaces' C++ typesupport.

    Args:
        name: C++ typesupport library target name
        group: interface group name (i.e. ROS 2 package name)
        interfaces: interface definition files
        typesupports: a mapping of available, vendor-specific
            C++ typesupport libraries, from typesupport name to
            library target label.
        includes: optional interface definition includes
        deps: optional library dependencies
        cc_binary_rule: optional cc_binary() rule override

    Additional keyword arguments are those common to all rules.
    """
    include, root = _deduce_source_paths(group, "typesupport/cpp")

    generated_cc_sources = []
    for ifc in interfaces:
        parent, basename = _deduce_source_parts(ifc)
        generated_cc_sources.append(
            "{}/{}/{}__type_support.cpp".format(root, parent, basename))

    rosidl_generate_genrule(
        name = _make_private_name(name, "_gen"),
        generated_sources = generated_cc_sources,
        typesupports = [
            "cpp[typesupport_implementations:[{}]]"
            .format(",".join(typesupports))
        ],
        group = group,
        interfaces = interfaces,
        includes = includes,
        output_dir = root,
        **kwargs
    )

    cc_binary_rule(
        name = name,
        srcs = generated_cc_sources,
        data = typesupports.values(),
        includes = [include],
        linkshared = True,
        deps = deps + [
            _make_private_label(label, "_hdrs")
            for label in typesupports.values()
        ] + [
            REPOSITORY_ROOT + ":rosidl_runtime_c_cc",
            REPOSITORY_ROOT + ":rosidl_runtime_cpp_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_cpp_cc",
            REPOSITORY_ROOT + ":rosidl_typesupport_interface_cc",
        ],
        **kwargs
    )

def rosidl_cc_support(
    name, interfaces, deps, group = None,
    cc_binary_rule = native.cc_binary,
    cc_library_rule = native.cc_library,
    **kwargs
):
    """
    Generates and builds C++ ROS 2 interfaces.

    To depend on C++ interfaces, use the `<name>_cc` target.

    Args:
        name: interface group name, used as prefix for target names
        interfaces: interface definition files
        deps: optional interface group dependencies
        group: optional interface group name override, useful when
            target name cannot be forced to match the intended package
            name for these interfaces
        cc_binary_rule: optional cc_binary() rule override
        cc_library_rule: optional cc_library() rule override

    Additional keyword arguments are those common to all rules.
    """
    rosidl_cc_library(
        name = _make_private_name(name, "__rosidl_cpp"),
        group = group or name,
        interfaces = interfaces,
        includes = [_make_public_label(dep, "_defs") for dep in deps],
        deps = [_make_public_label(dep, "_cc") for dep in deps],
        cc_library_rule = cc_library_rule,
        **kwargs
    )

    typesupports = {}
    # NOTE: typesupport binary files must not have any leading
    # underscores or C++ generated code will not be able to
    # dlopen it.
    if "rosidl_typesupport_introspection_cpp" in AVAILABLE_TYPESUPPORT_LIST:
        rosidl_typesupport_introspection_cc_library(
            name = _make_public_name(name, "__rosidl_typesupport_introspection_cpp"),
            group = group or name,
            interfaces = interfaces,
            includes = [_make_public_label(dep, "_defs") for dep in deps],
            deps = [_make_private_label(name, "__rosidl_cpp")] + [
                _make_public_label(dep, "_cc") for dep in deps],
            cc_binary_rule = cc_binary_rule,
            cc_library_rule = cc_library_rule,
            **kwargs
        )
        typesupports["rosidl_typesupport_introspection_cpp"] = \
            _make_public_label(name, "__rosidl_typesupport_introspection_cpp")

    if "rosidl_typesupport_fastrtps_cpp" in AVAILABLE_TYPESUPPORT_LIST:
        rosidl_typesupport_fastrtps_cc_library(
            name = _make_public_name(name, "__rosidl_typesupport_fastrtps_cpp"),
            group = group or name,
            interfaces = interfaces,
            includes = [_make_public_label(dep, "_defs") for dep in deps],
            deps = [_make_private_label(name, "__rosidl_cpp")] + [
                _make_public_label(dep, "_cc") for dep in deps],
            cc_binary_rule = cc_binary_rule,
            cc_library_rule = cc_library_rule,
            **kwargs
        )
        typesupports["rosidl_typesupport_fastrtps_cpp"] =  \
            _make_public_label(name, "__rosidl_typesupport_fastrtps_cpp")

    rosidl_typesupport_cc_library(
        name = _make_public_name(name, "__rosidl_typesupport_cpp"),
        typesupports = typesupports,
        group = group or name,
        interfaces = interfaces,
        includes = [_make_public_label(dep, "_defs") for dep in deps],
        deps = [_make_private_label(name, "__rosidl_cpp")] + [
            _make_public_label(dep, "_cc") for dep in deps],
        cc_binary_rule = cc_binary_rule,
        **kwargs
    )

    cc_library_rule(
        name = _make_public_name(name, "_cc"),
        srcs = [
            _make_public_label(name, "__rosidl_typesupport_cpp")
        ] + typesupports.values(),
        deps = [_make_private_label(name, "__rosidl_cpp")],
        linkstatic = True,
        **kwargs
    )

def rosidl_py_support(
    name, interfaces, deps, group = None,
    cc_binary_rule = native.cc_binary,
    cc_library_rule = native.cc_library,
    py_library_rule = native.py_library,
    **kwargs
):
    """
    Generates and builds Python ROS 2 interfaces.

    To depend on Python interfaces, use the `<name>_py` target.

    Args:
        name: interface group name, used as prefix for target names
        interfaces: interface definition files
        deps: optional interface group dependencies
        group: optional interface group name override, useful when
            target name cannot be forced to match the intended package
            name for these interfaces
        cc_binary_rule: optional cc_binary() rule override
        cc_library_rule: optional cc_library() rule override
        py_library_rule: optional py_library() rule override

    Additional keyword arguments are those common to all rules.
    """
    rosidl_c_library(
        name = _make_private_name(name, "__rosidl_c"),
        group = group or name,
        interfaces = interfaces,
        includes = [_make_public_label(dep, "_defs") for dep in deps],
        deps = [_make_public_label(dep, "_c") for dep in deps],
        cc_library_rule = cc_library_rule,
        **kwargs
    )

    typesupports = {}
    # NOTE: typesupport binary files must not have any leading
    # underscores or C generated code will not be able to
    # dlopen it.
    if "rosidl_typesupport_introspection_c" in AVAILABLE_TYPESUPPORT_LIST:
        rosidl_typesupport_introspection_c_library(
            name = _make_public_name(name, "__rosidl_typesupport_introspection_c"),
            group = group or name,
            interfaces = interfaces,
            includes = [_make_public_label(dep, "_defs") for dep in deps],
            deps = [_make_private_label(name, "__rosidl_c")] + [
                _make_public_label(dep, "_c") for dep in deps],
            cc_binary_rule = cc_binary_rule,
            cc_library_rule = cc_library_rule,
            **kwargs
        )
        typesupports["rosidl_typesupport_introspection_c"] = \
            _make_public_label(name, "__rosidl_typesupport_introspection_c")

    if "rosidl_typesupport_fastrtps_c" in AVAILABLE_TYPESUPPORT_LIST:
        rosidl_typesupport_fastrtps_c_library(
            name = _make_public_name(name, "__rosidl_typesupport_fastrtps_c"),
            group = group or name,
            interfaces = interfaces,
            includes = [_make_public_label(dep, "_defs") for dep in deps],
            deps = [_make_private_label(name, "__rosidl_c")] + [
                _make_public_label(dep, "_c") for dep in deps],
            cc_binary_rule = cc_binary_rule,
            cc_library_rule = cc_library_rule,
            **kwargs
        )
        typesupports["rosidl_typesupport_fastrtps_c"] = \
            _make_public_label(name, "__rosidl_typesupport_fastrtps_c")

    rosidl_typesupport_c_library(
        name = _make_public_name(name, "__rosidl_typesupport_c"),
        typesupports = typesupports,
        group = group or name,
        interfaces = interfaces,
        includes = [_make_public_label(dep, "_defs") for dep in deps],
        deps = [_make_private_label(name, "__rosidl_c")] + [
            _make_public_label(dep, "_c") for dep in deps],
        cc_binary_rule = cc_binary_rule,
        **kwargs
    )
    typesupports["rosidl_typesupport_c"] = \
        _make_public_label(name, "__rosidl_typesupport_c")

    cc_library_rule(
        name = _make_public_name(name, "_c"),
        srcs = typesupports.values(),
        deps = [
            _make_private_label(name, "__rosidl_c")
        ] + typesupports.values(),
        linkstatic = True,
        **kwargs
    )

    rosidl_py_library(
        name = _make_public_name(name, "_py"),
        typesupports = typesupports,
        group = group or name,
        interfaces = interfaces,
        includes = [_make_public_label(dep, "_defs") for dep in deps],
        py_deps = [_make_public_label(dep, "_py") for dep in deps],
        c_deps = [_make_public_label(name, "_c")] + [
            _make_public_label(dep, "_c") for dep in deps],
        cc_binary_rule = cc_binary_rule,
        cc_library_rule = cc_library_rule,
        py_library_rule = py_library_rule,
        **kwargs
    )

def rosidl_interfaces_group(
    name, interfaces, deps = [], group = None,
    cc_binary_rule = native.cc_binary,
    cc_library_rule = native.cc_library,
    py_library_rule = native.py_library,
    **kwargs
):
    """
    Generates and builds C++ and Python ROS 2 interfaces.

    To depend on C++ interfaces, use the `<name>_cc` target.
    To depend on Python interfaces, use the `<name>_py` target.

    Args:
        name: interface group name, used as prefix for target names
        interfaces: interface definition files
        deps: optional interface group dependencies
        group: optional interface group name override, useful when
            target name cannot be forced to match the intended package
            name for these interfaces
        cc_binary_rule: optional cc_binary() rule override
        cc_library_rule: optional cc_library() rule override
        py_library_rule: optional py_library() rule override

    Additional keyword arguments are those common to all rules.
    """
    rosidl_definitions_filegroup(
        name = _make_public_name(name, "_defs"),
        group = group or name,
        interfaces = interfaces,
        includes = [_make_public_label(dep, "_defs") for dep in deps],
        **kwargs
    )

    rosidl_generate_ament_index_entry(
        name = name + "_ament_index",
        group = group or name,
        interfaces = [name + "_defs"] + interfaces,
    )

    rosidl_cc_support(
        name, interfaces, deps, group,
        cc_binary_rule = cc_binary_rule,
        cc_library_rule = cc_library_rule,
        **kwargs
    )

    rosidl_py_support(
        name, interfaces, deps, group,
        cc_binary_rule = cc_binary_rule,
        cc_library_rule = cc_library_rule,
        py_library_rule = py_library_rule,
        **kwargs
    )
