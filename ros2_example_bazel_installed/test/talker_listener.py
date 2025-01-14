import argparse

import rclpy
import rclpy.node
import std_msgs
from std_msgs.msg import Float64
from rclpy.executors import MultiThreadedExecutor

class Talker(rclpy.node.Node):
    def __init__(self, id):
        super().__init__('talker_' + str(id))
        self._publisher = self.create_publisher(
                std_msgs.msg.Float64, 'chatter', 10)
        self._timer = self.create_timer(0.1, self._timer_callback)
        self._id = id

    def _timer_callback(self):
        msg = std_msgs.msg.Float64()
        msg.data = float(self._id)
        self._publisher.publish(msg)

class Listener(rclpy.node.Node):
    def __init__(self, id):
        super().__init__('listener_' + str(id))
        self._subscription = self.create_subscription(
            std_msgs.msg.Float64,
            'chatter',
            self._topic_callback,
            10)
        timeout = self.declare_parameter('timeout', 2.0)
        self._timer = self.create_timer(
            timeout.value, self._timer_callback)
        self._expected_messages_received = 0
        self._id = id

    def _topic_callback(self, msg):
        assert msg.data == self._id, \
            f"I heard '{msg.data}' yet I was expecting '{self._id}'!"
        self._expected_messages_received += 1

    def _timer_callback(self):
        assert self._expected_messages_received > 0, \
            f"I did not hear '{self._id}' even once!"
        rclpy.shutdown()

def main():
    # This script launches a pair of a talker and a listener
    # that are bound to some numerical id. The talker publishes
    # the id, and the listener expectes that id in the msg.
    parser = argparse.ArgumentParser()
    parser.add_argument("--id", type=int, default=0)
    args = parser.parse_args()

    rclpy.init()

    executor = MultiThreadedExecutor()
    executor.add_node(Talker(args.id))
    executor.add_node(Listener(args.id))

    executor.spin()

if __name__ == "__main__":
    main()
