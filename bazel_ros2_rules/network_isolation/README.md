# Introduction
This directory contains tools and targets to isolate ros2 tests in this repository using linux network namespaces.
At its core, it uses the ``unshare()`` system call, and is meant to isolate ROS2 traffic. It creates a new user namespace,
new network namespace to prevent cross talk via the network, and new IPC namespace to prevent cross talk using shared memory.

The existing dload_shim is used to pass on the required argument and isolate the tests.

## Why do we need this ?
When running ROS2 tests in parallel, they might publish on the same topics and there might be cross talk between tests. RMW config or ROS domain id based isolation is possible,
but it does not scale well, due to limited ports and domain ids available. Linux namespaces provide a generic and a scalable way to solve this problem.

## Why not isolate individual targets instead of tests ?
Isolation using the namespace approach requires 3 namespaces, or "credentials" for processes to talk to each other, or be in the same realm : IPC, user and network namespaces.
Tests are more generic than individual targets, as the tests are free to fork() or run any number of processes they want, and they'll live in the same namespace.

If we do isolate a process 'A', how do we make sure the next process 'B' will live in the same namespace as 'A' (if they're meant to talk to each other) ? There needs to be an API to provide the above mentioned "credentials" to the new process
somehow, which out of scope of this feature for now.

# Targets
The logic lives in the following targets, which are meant to be used with the bazel rules ``ros_cc_test()`` and ``ros_py_test()`` :
* ``network_isolation_cc`` (cc_library): This is where the core logic lives, and the ``unshare()`` call is run.
* ``network_isolation_py.so``(cpython extension) : Python binding for the network isolation logic.
* ``network_isolation_py`` (py_library) : Importable python module for the network isolation logic.

Other than these, there is a standalone executable target called ``isolate`` which is meant to be used in a standalone way, and not with the
``ros_*_test()`` rules. It isolates the process in the first argument provided to it. This uses the same ``unshare()`` logic as ``network_isolation_cc``.

# How do we use this feature ?
There are 3 ways to use this feature :

## Using ``ros_cc_test()`` rule :
There is now an extra argument (``network_isolation``) available to the rule, so for e.g. we can modify a test in ``ros2_example_bazel_installed/ros2_example_apps/BUILD.bazel`` as : 

```
ros_cc_test(
    name = "talker_listener_cc_test",
    size = "small",
    srcs = ["test/talker_listener.cc"],
    rmw_implementation = "rmw_cyclonedds_cpp",
    network_isolation = True,
    deps = [
        ":listener_cc",
        ":talker_cc",
        "@ros2//:rclcpp_cc",
        "@ros2//:std_msgs_cc",
        "@ros2//resources/rmw_isolation:rmw_isolation_cc",
    ],
)
```

Whatever processes are spawned by this test will be contained in a namespace, and will be able to talk to each other, but not to any other ros nodes running outside of this test. 

## Using the ``ros_py_test()`` rule :
Similarly for the python test rule, we can add ``network_isolation`` to ``True``. Consider this section in ``drake_ros_examples/examples/iiwa_manipulator/BUILD.bazel`` :

```
ros_py_test(
    name = "iiwa_manipulator_test",
    network_isolation = True,
    srcs = ["test/iiwa_manipulator_test.py"],
    data = [
        ":iiwa_manipulator",
        ":iiwa_manipulator_py",
    ],
    main = "test/iiwa_manipulator_test.py",
    deps = [
        "@ros2//resources/bazel_ros_env:bazel_ros_env_py",
    ],
)
```

Similarly, the ros nodes spawned in this test can only talk to each other, and not other nodes running on the system at the same time, even if you have multiple instances of this test running in different terminals.


## Using the ``isolate`` target:
As mentioned before, the ``isolate`` target is meant to be used in a generic way and will isolate **any** process provided to it provided it is available in the bazel sandbox, for e.g. : 
```
cd bazel_ros2_rules
bazel run //network_isolation:isolate -- /bin/bash -c "echo HELLO_WORLD"
```

Here, the bash command for printing "HELLO_WORLD" is isolated, and can be replaced with any ros2 command. For instance, if you have ros2 humble installed
outside of drake-ros, using debs, you could run a publisher and subscriber that would be isolated from each other : 

Lets start the talker from the demo nodes package:
```
bazel run //network_isolation:isolate -- /bin/bash -c "source /opt/ros/humble/setup.bash && export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp && ros2 run demo_nodes_cpp talker"
```

In another terminal, run :
```
bazel run //network_isolation:isolate -- /bin/bash -c "source /opt/ros/humble/setup.bash && export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp && ros2 run demo_nodes_cpp listener"
```

These 2 processes should not be able to talk to each other, when used with ``isolate``.

# Testing this feature in CI
Among the 3 ways to use this feature, as far as CI is concerned, there is a test for the ``isolate`` target mechanism.

The test target is called ``network_isolation_test`` and uses ``ros2_example_bazel_installed/test/network_isolation_test.py`` to run 5 (by default) talker-listener pairs which have an id attached to them.
They all publish and listen on the same topic at the same time, and expect no cross talk between the pairs. The number of pairs can be changed using the ``--id`` cmdline argument if needed.

The other 2 ways to use this feature, using ``ros_*_test()`` rules, run on tests, and not on executable targets, so *writing a test for a test* seems like an anti-pattern. The ``isolate`` target uses the same logic, so should be sufficient for testing.
