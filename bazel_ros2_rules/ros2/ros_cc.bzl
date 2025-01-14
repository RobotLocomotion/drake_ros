# -*- python -*-

load("//tools:ament_index.bzl", "AmentIndex")
load(
    "//tools:common.bzl",
    "incorporate_rmw_implementation",
)
load(
    "//tools:dload_cc.bzl",
    "dload_cc_ldwrap",
    "dload_cc_reexec",
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

def ros_cc_binary(
        name,
        rmw_implementation = None,
        cc_binary_rule = native.cc_binary,
        cc_library_rule = native.cc_library,
        shim = "reexec",
        **kwargs):
    """
    Builds a C/C++ binary and wraps it with a shim that will inject the minimal
    runtime environment necessary for execution when depending on targets from
    this ROS 2 local repository.

    Equivalent to the cc_binary() rule, which this rule decorates.

    Args:
        name: C/C++ binary target name
        rmw_implementation: optional RMW implementation to run against
        cc_binary_rule: optional cc_binary() rule override
        cc_library_rule: optional cc_library() rule override
        shim: optional tactic to use for shimming ("reexec" or "ldwrap")

    Additional keyword arguments are forwarded to the `cc_binary_rule`.

    The shim tactic of "reexec" will compile two programs: one with the
    requested `name` which is just a tiny wrapper that sets environment
    variables and then replaces itself with the second program (which has
    your actual code). This is the most robust shimming option, with the
    downside of having two programs and extra process indirection.

    The shim tactic of "ldwrap" will compile just one program, using the linker
    flag `--wrap` to insert extra code prior to main(). This has the benefit of
    no subprocess indirection, with the following caveats:
      * It does not support programs that use the 3-argument form of main,
        (where environment variables are passed as a third argument).
      * It does not support programs where the main() function is defined in a
        cc_library. Always pass the main() function definition in `srcs`.
        Note that unit test frameworks often use a library-based main.
    """

    # When creating a shared library ("libfoo.so"), don't do anything special.
    if kwargs.get("linkshared", False):
        native.cc_binary(name = name, **kwargs)
        return

    # Prepare the list of environment actions.
    env_changes = dict(RUNTIME_ENVIRONMENT)
    if rmw_implementation:
        kwargs, env_changes = \
            incorporate_rmw_implementation(
                kwargs,
                env_changes,
                rmw_implementation = rmw_implementation,
            )

    # Declare an unshimmed, manual binary. For the "reexec" case, this will be
    # the program that we reexec into. For the "ldwrap" case, this might still
    # be helpful for debugging, but anyway is required to consolidate the
    # program's ROS-Bazel aspects into one place.
    noshim_name = "_{}_noshim".format(name)
    noshim_kwargs = dict(kwargs)
    noshim_kwargs.update(visibility = ["//visibility:private"])
    noshim_tags = noshim_kwargs.pop("tags", [])
    if "manual" not in noshim_tags:
        noshim_tags = noshim_tags + ["manual"]
    cc_binary_rule(
        name = noshim_name,
        tags = noshim_tags,
        **noshim_kwargs
    )

    if shim == "reexec":
        # Codegen a main function that performs the actions.
        main_name_cc = "_{}_reexec_main.cc".format(name)
        dload_cc_reexec(
            name = main_name_cc,
            target = ":" + noshim_name,
            env_changes = env_changes,
            testonly = kwargs.get("testonly", False),
            tags = [
                # Only codegen when needed as a dependency.
                "manual",
            ],
            visibility = ["//visibility:private"],
        )

        # Compile it into a binary.
        shim_kwargs = filter_to_only_common_kwargs(kwargs)
        shim_tags = shim_kwargs.pop("tags", [])
        if "nolint" not in shim_tags:
            # Don't check generated code.
            shim_tags = shim_tags + ["nolint"]
        shim_kwargs.update(
            srcs = [main_name_cc],
            data = [":" + noshim_name],
            deps = ["@bazel_ros2_rules//ros2:dload_shim_cc"],
            tags = shim_tags,
        )
        cc_binary_rule(name = name, **shim_kwargs)
    elif shim == "ldwrap":
        # Codegen a main function that performs the actions.
        main_name = "_{}_ldwrap_main".format(name)
        main_cc = main_name + ".cc"
        dload_cc_ldwrap(
            name = main_cc,
            target = ":" + noshim_name,
            env_changes = env_changes,
            testonly = kwargs.get("testonly", False),
            tags = [
                # Only codegen when needed as a dependency.
                "manual",
            ],
            visibility = ["//visibility:private"],
        )

        # Link the main function into a static library.
        main_kwargs = filter_to_only_common_kwargs(kwargs)
        main_kwargs.update(visibility = ["//visibility:private"])
        main_tags = main_kwargs.pop("tags", [])
        if "nolint" not in main_tags:
            # Don't check generated code.
            main_tags = main_tags + ["nolint"]
        cc_library_rule(
            name = main_name,
            srcs = [main_cc],
            linkstatic = True,
            deps = ["@bazel_ros2_rules//ros2:dload_shim_cc"],
            tags = main_tags,
            **main_kwargs
        )

        # Link the final binary, using the codegen'd main in lieu of the
        # original main.
        kwargs.update(
            deps = kwargs.get("deps", []) + [":" + main_name],
            linkopts = kwargs.get("linkopts", []) + ["-Wl,--wrap=main"],
        )
        cc_binary_rule(name = name, **kwargs)
    else:
        fail("Unsupported shim=" + str(shim))

def ros_cc_test(
        name,
        rmw_implementation = None,
        cc_binary_rule = native.cc_binary,
        cc_library_rule = native.cc_library,
        cc_test_rule = native.cc_test,
        network_isolation = False,
        **kwargs):
    """
    Builds a C/C++ test and wraps it with a shim that will inject the minimal
    runtime environment necessary for execution when depending on targets from
    this ROS 2 local repository.

    Equivalent to the cc_test() rule.

    Args:
        name: C/C++ test target name
        rmw_implementation: optional RMW implementation to run against
        cc_binary_rule: optional cc_binary() rule override.
        cc_library_rule: optional cc_library() rule override (currently unused)
        cc_test_rule: optional cc_test() rule override

    Additional keyword arguments are forwarded to the `cc_test_rule` and to the
    `cc_binary_rule` (minus the test specific ones).
    """
    if "shim" in kwargs:
        fail("ros_cc_test does not support the shim= option yet.")

    noshim_name = "_" + name + "_noshim"
    noshim_kwargs = remove_test_specific_kwargs(kwargs)
    noshim_kwargs.update(testonly = True)
    shim_env_changes = dict(RUNTIME_ENVIRONMENT)
    if rmw_implementation:
        noshim_kwargs, test_env_changes = \
            incorporate_rmw_implementation(
                noshim_kwargs,
                shim_env_changes,
                rmw_implementation = rmw_implementation,
            )

    cc_binary_rule(
        name = noshim_name,
        **noshim_kwargs
    )

    shim_name = "_" + name + "_shim.cc"
    shim_kwargs = filter_to_only_common_kwargs(kwargs)
    shim_kwargs.update(testonly = True)
    dload_cc_reexec(
        name = shim_name,
        target = ":" + noshim_name,
        env_changes = shim_env_changes,
        network_isolation = network_isolation,
        **shim_kwargs
    )

    kwargs.update(
        srcs = [shim_name],
        data = [":" + noshim_name],
        deps = ["@bazel_ros2_rules//ros2:dload_shim_cc"],
        tags = ["nolint"] + kwargs.get("tags", []),
    )
    if network_isolation:
        kwargs["deps"].append(
            "@bazel_ros2_rules//network_isolation:network_isolation_cc",
        )
    cc_test_rule(name = name, **kwargs)
