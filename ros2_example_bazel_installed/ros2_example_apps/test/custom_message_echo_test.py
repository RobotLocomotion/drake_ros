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
    talker_bin = manifest.Rlocation(
        "ros2_example_bazel_installed/ros2_example_apps/simple_talker")

    timeout = 5.0
    topic_echo = subprocess.Popen([
        ros2_bin, "topic", "echo", "--once",
        "/status", "ros2_example_apps_msgs/msg/Status"])
    talker = subprocess.Popen([talker_bin])
    topic_echo.wait(timeout=timeout)
    assert topic_echo.returncode == 0
    talker.kill()

    interfaces = subprocess.run(
        [ros2_bin, "interface", "list"],
        check=True, text=True, stdout=subprocess.PIPE,
    ).stdout
    print(interfaces)
    assert "ros2_example_apps_msgs/msg/Status" in interfaces

    print("[ Done ]")


if __name__ == "__main__":
    main()
