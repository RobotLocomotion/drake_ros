"""
Test sentinel environment variable handling in dload shims.

These tests check that shimmed executables set environment variables normally,
but skip modifying environment variables when a sentinel variable is set.
This is used to avoid modifying environment variables twice when a binary
wrapped with a dload shim is called in a subprocess of another binary that was
also wrapped in a dload shim.
"""

import copy
import json
import os
import subprocess
import unittest

from bazel_tools.tools.python.runfiles import runfiles

SHIM_SENTINEL = "_BAZEL_ROS2_RULES_SHIMMED"


def run_bazel_target(target, env, *args):
    """Run a bazel executable target and return stdout."""
    r = runfiles.Create()
    env = copy.deepcopy(env)
    # The binaries used by this test need their runfiles to run successfully.
    # Give them the test's runfiles, because they include the binaries'
    # runfiles, and are the only ones guaranteed to exist.
    # See RobotLocomotion/drake-ros#106 for more info.
    env.update(r.EnvVars())
    p = subprocess.Popen(
        [r.Rlocation(target)] + list(args),
        env=env,
        stdout=subprocess.PIPE)
    return p.communicate()[0].decode()


class TestShim(unittest.TestCase):

    def setUp(self):
        assert SHIM_SENTINEL not in os.environ

        self._mock_shimmed_env = {SHIM_SENTINEL: ""}

    def test_shimmed_once_cc(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_cc", {})
        result = json.loads(stdout)
        self.assertTrue(result['shimmed'])
        self.assertTrue(result['AMENT_PREFIX_PATH present'])

    def test_mock_shimmed_twice_cc(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_cc",
            self._mock_shimmed_env)
        result = json.loads(stdout)
        self.assertTrue(result['shimmed'])
        self.assertFalse(result['AMENT_PREFIX_PATH present'])

    def test_shimmed_once_py(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_py", {})
        result = json.loads(stdout)
        self.assertTrue(result['shimmed'])
        self.assertTrue(result['AMENT_PREFIX_PATH present'])

    def test_mock_shimmed_twice_py(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_py",
            self._mock_shimmed_env)
        result = json.loads(stdout)
        self.assertTrue(result['shimmed'])
        self.assertFalse(result['AMENT_PREFIX_PATH present'])


if __name__ == '__main__':
    unittest.main()
