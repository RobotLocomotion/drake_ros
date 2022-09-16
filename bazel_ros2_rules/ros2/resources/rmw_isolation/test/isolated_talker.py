import os

import rclpy
import rclpy.node
import std_msgs.msg


class IsolatedTalker(rclpy.node.Node):

    def __init__(self, uuid):
        super().__init__('isolated_talker')
        self._publisher = self.create_publisher(
            std_msgs.msg.String, 'uuid', 10)
        self._timer = self.create_timer(0.1, self._timer_callback)
        self._uuid = uuid

    def _timer_callback(self):
        msg = std_msgs.msg.String()
        msg.data = self._uuid
        self._publisher.publish(msg)


def main():
    if 'TEST_TMPDIR' in os.environ:
        from rmw_isolation import isolate_rmw_by_path
        isolate_rmw_by_path(os.environ['TEST_TMPDIR'])

    rclpy.init()

    uuid = os.environ.get('TEST_TMPDIR', 'none')
    try:
        rclpy.spin(IsolatedTalker(uuid))
    # TODO(hidmic): simplify `except` blocks as follows:
    #
    #   except KeyboardInterrupt:
    #       pass
    #   finally:
    #       rclpy.try_shutdown()
    #
    # when rclpy 3.2.0 is rolled out (or any rclpy version
    # including https://github.com/ros2/rclpy/pull/868 is).
    except rclpy.executors.ExternalShutdownException:
        pass
    except KeyboardInterrupt:
        rclpy.shutdown()
    except Exception:
        rclpy.shutdown()
        raise


if __name__ == '__main__':
    main()
