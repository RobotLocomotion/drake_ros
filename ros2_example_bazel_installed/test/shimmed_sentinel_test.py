import json
import os
import subprocess
import unittest

from bazel_tools.tools.python.runfiles import runfiles

SHIM_SENTINEL = "_BAZEL_ROS2_RULES_SHIMMED"


def run_bazel_target(target, *args, env=None):
    """Run a bazel executable target and return stdout."""
    r = runfiles.Create()
    if env is None:
        env = {}
    p = subprocess.Popen(
        [r.Rlocation(target)] + list(args),
        env=env,
        stdout=subprocess.PIPE)
    return p.communicate()[0].decode()


class TestShim(unittest.TestCase):

    def setUp(self):
        assert SHIM_SENTINEL not in os.environ

    def test_shimmed_once_cc(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_cc")
        result = json.loads(stdout)
        self.assertTrue(result['shimmed'])
        self.assertTrue(result['AMENT_PREFIX_PATH present'])

    def test_mock_shimmed_twice_cc(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_cc",
            env={SHIM_SENTINEL: ""})
        result = json.loads(stdout)
        self.assertTrue(result['shimmed'])
        self.assertFalse(result['AMENT_PREFIX_PATH present'])

    def test_shimmed_once_py(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_py")
        result = json.loads(stdout)
        self.assertTrue(result['shimmed'])
        self.assertTrue(result['AMENT_PREFIX_PATH present'])

    def test_mock_shimmed_twice_py(self):
        stdout = run_bazel_target(
            "ros2_example_bazel_installed/shimmed_sentinel_py",
            env={SHIM_SENTINEL: ""})
        result = json.loads(stdout)
        self.assertTrue(result['shimmed'])
        self.assertFalse(result['AMENT_PREFIX_PATH present'])


if __name__ == '__main__':
    unittest.main()
