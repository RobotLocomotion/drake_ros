#!/usr/bin/env python3

import subprocess
import unittest

from bazel_tools.tools.python.runfiles import runfiles


def run_bazel_target(target, *args, env=None):
    """Run a bazel executable target and return stdout."""
    r = runfiles.Create()
    if env is None:
        env = {}
    env.update(r.EnvVars())
    p = subprocess.Popen(
        [r.Rlocation(target)] + list(args),
        env=env,
        stdout=subprocess.PIPE)
    return p.communicate()[0].decode()


class TestShim(unittest.TestCase):

    def test_shimmed_once_cc(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_cc")
        self.assertEqual(
            "shimmed: yes AMENT_PREFIX_PATH present: yes",
            stdout)

    def test_mock_shimmed_twice_cc(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_cc",
            env={"_BAZEL_ROS2_RULES_SHIMMED": ""})
        self.assertEqual(
            "shimmed: yes AMENT_PREFIX_PATH present: no",
            stdout)

    def test_shimmed_once_py(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_py")
        self.assertEqual(
            "shimmed: yes AMENT_PREFIX_PATH present: yes",
            stdout.strip())

    def test_mock_shimmed_twice_py(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_py",
            env={"_BAZEL_ROS2_RULES_SHIMMED": ""})
        self.assertEqual(
            "shimmed: yes AMENT_PREFIX_PATH present: no",
            stdout.strip())


if __name__ == '__main__':
    unittest.main()
