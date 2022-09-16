import rclpy
import rclpy.node

from ros2_example_apps_msgs.msg import Status


class StatusPublisher(rclpy.node.Node):
    def __init__(self):
        super().__init__("test")
        self._pub = self.create_publisher(Status, "/status", 10)
        self._timer = self.create_timer(0.1, self._callback)

    def _callback(self):
        self._pub.publish(Status())


def main():
    rclpy.init()
    try:
        rclpy.spin(StatusPublisher())
    finally:
        rclpy.shutdown()


if __name__ == "__main__":
    main()
