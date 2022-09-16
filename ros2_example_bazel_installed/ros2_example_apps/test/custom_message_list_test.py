import os
import subprocess

from bazel_tools.tools.python.runfiles import runfiles
from rmw_isolation import isolate_rmw_by_path


def main():
    if "TEST_TMPDIR" in os.environ:
        isolate_rmw_by_path(os.environ["TEST_TMPDIR"])
        os.environ["ROS_HOME"] = os.path.join(os.environ["TEST_TMPDIR"])

    manifest = runfiles.Create()
    ros2_bin = manifest.Rlocation("ros2/ros2")

    interfaces = subprocess.run(
        [ros2_bin, "interface", "list"],
        check=True, text=True, stdout=subprocess.PIPE,
    ).stdout
    print(interfaces)
    assert "ros2_example_apps_msgs/msg/Status" in interfaces

    print("[ Done ]")


if __name__ == "__main__":
    main()
