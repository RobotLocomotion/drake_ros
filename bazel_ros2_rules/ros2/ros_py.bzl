# -*- python -*-

load(
    "//tools:dload_py.bzl",
    "dload_py_shim"
)
load(
    "//tools:kwargs.bzl",
    "filter_to_only_common_kwargs",
    "remove_test_specific_kwargs"
)
load(
    "//tools:common.bzl",
    "incorporate_rmw_implementation",
)
load(
    ":distro.bzl",
    "RUNTIME_ENVIRONMENT"
)

def ros_import_binary(
    name, executable, rmw_implementation = None, py_binary_rule = native.py_binary, **kwargs
):
    """
    Imports an existing executable by wrapping it with a Python shim that will inject the
    minimal runtime environment necessary for execution when depending on this ROS 2 local
    repository. Imported executables need not be Python -- binary executables will work the
    same.

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
                kwargs, env_changes,
                rmw_implementation = rmw_implementation
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
            "@bazel_ros2_rules//ros2:dload_shim_py"]
    )
    py_binary_rule(name = name, **kwargs)

def ros_py_binary(
    name, rmw_implementation = None, py_binary_rule = native.py_binary, **kwargs
):
    """
    Builds a Python binary and wraps it with a shim that will inject the minimal
    runtime environment necessary for execution when depending on targets from
    this ROS 2 local repository.

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
                noshim_kwargs, shim_env_changes,
                rmw_implementation = rmw_implementation
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
        tags = ["nolint"] + kwargs.get("tags", [])
    )
    py_binary_rule(name = name, **kwargs)

def ros_py_test(
    name, rmw_implementation = None,
    py_binary_rule = native.py_binary,
    py_test_rule = native.py_test, **kwargs
):
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
                noshim_kwargs, shim_env_changes,
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
        **shim_kwargs
    )

    kwargs.update(
        srcs = [shim_name],
        main = shim_name,
        data = [":" + noshim_name],
        deps = ["@bazel_ros2_rules//ros2:dload_shim_py"],
        tags = ["nolint"] + kwargs.get("tags", [])
    )
    py_test_rule(name = name, **kwargs)
