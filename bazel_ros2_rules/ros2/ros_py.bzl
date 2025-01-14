# -*- python -*-

load(
    "//tools:common.bzl",
    "incorporate_rmw_implementation",
)
load(
    "//tools:dload_py.bzl",
    "dload_py_shim",
)
load(
    "//tools:kwargs.bzl",
    "filter_to_only_common_kwargs",
    "remove_test_specific_kwargs",
)
load(
    ":distro.bzl",
    "RUNTIME_ENVIRONMENT",
)

def ros_import_binary(
        name,
        executable,
        rmw_implementation = None,
        py_binary_rule = native.py_binary,
        **kwargs):
    """
    Imports an existing executable by wrapping it with a Python shim that will
    inject the minimal runtime environment necessary for execution when
    depending on this ROS 2 local repository. Imported executables need not be
    Python -- binary executables will work the same.

    Akin to the cc_import() rule.

    Args:
        name: imported executable target name
        executable: path to an executable file
        rmw_implementation: optional RMW implementation to run against
        py_binary_rule: optional py_binary() rule override

    Additional keyword arguments are forwarded to the `py_binary_rule`.
    """

    env_changes = dict(RUNTIME_ENVIRONMENT)
    if rmw_implementation:
        kwargs, env_changes = \
            incorporate_rmw_implementation(
                kwargs,
                env_changes,
                rmw_implementation = rmw_implementation,
            )

    shim_name = "_" + name + "_shim.py"
    shim_kwargs = filter_to_only_common_kwargs(kwargs)
    dload_py_shim(
        name = shim_name,
        target = executable,
        env_changes = env_changes,
        **shim_kwargs
    )

    kwargs.update(
        srcs = [shim_name],
        main = shim_name,
        tags = ["nolint"] + kwargs.get("tags", []),
        data = [executable] + kwargs.get("data", []),
        deps = kwargs.get("deps", []) + [
            "@bazel_ros2_rules//ros2:dload_shim_py",
        ],
    )
    py_binary_rule(name = name, **kwargs)

def ros_py_binary(
        name,
        rmw_implementation = None,
        py_binary_rule = native.py_binary,
        **kwargs):
    """
    Builds a Python binary and wraps it with a shim that will inject the
    minimal runtime environment necessary for execution when depending on
    targets from this ROS 2 local repository.

    Equivalent to the py_binary() rule, which this rule decorates.

    Args:
        name: Python binary target name
        rmw_implementation: optional RMW implementation to run against
        py_binary_rule: optional py_binary() rule override

    Additional keyword arguments are forwarded to the `py_binary_rule`.
    """

    noshim_name = "_" + name + "_noshim"
    noshim_kwargs = dict(kwargs)
    if "main" not in noshim_kwargs:
        noshim_kwargs["main"] = name + ".py"
    shim_env_changes = dict(RUNTIME_ENVIRONMENT)

    if rmw_implementation:
        noshim_kwargs, shim_env_changes = \
            incorporate_rmw_implementation(
                noshim_kwargs,
                shim_env_changes,
                rmw_implementation = rmw_implementation,
            )

    py_binary_rule(
        name = noshim_name,
        **noshim_kwargs
    )

    shim_name = "_" + name + "_shim.py"
    shim_kwargs = filter_to_only_common_kwargs(kwargs)
    dload_py_shim(
        name = shim_name,
        target = ":" + noshim_name,
        env_changes = shim_env_changes,
        **shim_kwargs
    )

    kwargs.update(
        srcs = [shim_name],
        main = shim_name,
        data = [":" + noshim_name],
        deps = [
            "@bazel_ros2_rules//ros2:dload_shim_py",
            ":" + noshim_name,  # Support py_binary being used a dependency
        ],
        tags = ["nolint"] + kwargs.get("tags", []),
    )
    py_binary_rule(name = name, **kwargs)

def _add_deps(existing, new):
    deps = list(existing)
    for dep in new:
        if dep not in deps:
            deps.append(dep)
    return deps

def _generate_file_impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.write(out, ctx.attr.content, ctx.attr.is_executable)
    return [DefaultInfo(
        files = depset([out]),
        data_runfiles = ctx.runfiles(files = [out]),
    )]

_generate_file = rule(
    attrs = {
        "content": attr.string(mandatory = True),
        "is_executable": attr.bool(default = False),
    },
    output_to_genfiles = True,
    implementation = _generate_file_impl,
)

_LAUNCH_PY_TEMPLATE = """
import os
import sys

from bazel_ros_env import Rlocation

assert __name__ == "__main__"
launch_file = Rlocation({launch_respath})
ros2_bin = Rlocation("ros2/ros2")
args = [ros2_bin, "launch", launch_file] + sys.argv[1:]
os.execv(ros2_bin, args)
"""

def _make_respath(relpath, workspace_name):
    repo = native.repository_name()
    if repo == "@":
        if workspace_name == None:
            fail(
                "Please provide `ros_launch(*, workspace_name)` so that " +
                "paths can be resolved properly",
            )
        repo = workspace_name
    pkg = native.package_name()
    if pkg != "":
        pieces = [repo, pkg, relpath]
    else:
        pieces = [repo, relpath]
    return "/".join(pieces)

def ros_launch(
        name,
        launch_file,
        args = [],
        data = [],
        deps = [],
        visibility = None,
        # TODO(eric.cousineau): Remove this once Bazel provides a way to tell
        # runfiles.py to use "this repository" in a way that doesn't require
        # bespoke information.
        workspace_name = None,
        **kwargs):
    main = "{}_roslaunch_main.py".format(name)
    launch_respath = _make_respath(launch_file, workspace_name)

    content = _LAUNCH_PY_TEMPLATE.format(
        launch_respath = repr(launch_respath),
    )
    _generate_file(
        name = main,
        content = content,
        visibility = ["//visibility:private"],
    )

    deps = _add_deps(
        deps,
        [
            "@ros2//:ros2",
            "@ros2//resources/bazel_ros_env:bazel_ros_env_py",
        ],
    )
    data = data + [launch_file]

    ros_py_binary(
        name = name,
        main = main,
        deps = deps,
        srcs = [main],
        data = data,
        visibility = visibility,
        **kwargs
    )

def ros_py_test(
        name,
        rmw_implementation = None,
        py_binary_rule = native.py_binary,
        py_test_rule = native.py_test,
        network_isolation = False,
        **kwargs):
    """
    Builds a Python test and wraps it with a shim that will inject the minimal
    runtime environment necessary for execution when depending on targets from
    this ROS 2 local repository.

    Equivalent to the py_test() rule, which this rule decorates.

    Args:
        name: Python test target name
        rmw_implementation: optional RMW implementation to run against
        py_binary_rule: optional py_binary() rule override
        py_test_rule: optional py_test() rule override

    Additional keyword arguments are forwarded to the `py_test_rule` and to the
    `py_binary_rule` (minus the test specific ones).
    """
    noshim_name = "_" + name + "_noshim"
    noshim_kwargs = remove_test_specific_kwargs(kwargs)
    noshim_kwargs.update(testonly = True)
    shim_env_changes = dict(RUNTIME_ENVIRONMENT)
    if rmw_implementation:
        noshim_kwargs, shim_env_changes = \
            incorporate_rmw_implementation(
                noshim_kwargs,
                shim_env_changes,
                rmw_implementation = rmw_implementation,
            )

    py_binary_rule(
        name = noshim_name,
        **noshim_kwargs
    )

    shim_name = "_" + name + "_shim.py"
    shim_kwargs = filter_to_only_common_kwargs(kwargs)
    shim_kwargs.update(testonly = True)
    dload_py_shim(
        name = shim_name,
        target = ":" + noshim_name,
        env_changes = shim_env_changes,
        network_isolation = network_isolation,
        **shim_kwargs
    )

    kwargs.update(
        srcs = [shim_name],
        main = shim_name,
        data = [":" + noshim_name],
        deps = ["@bazel_ros2_rules//ros2:dload_shim_py"],
        tags = ["nolint"] + kwargs.get("tags", []),
    )
    if network_isolation:
        kwargs["deps"].append(
            "@bazel_ros2_rules//network_isolation:network_isolation_py",
        )
    py_test_rule(name = name, **kwargs)
