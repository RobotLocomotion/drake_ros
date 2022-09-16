#!/usr/bin/env python3

import random

import rclpy
import rclpy.action
import rclpy.duration
import rclpy.node
import rclpy.qos

import ros2_example_apps_msgs.msg
import ros2_example_common_msgs.action
import ros2_example_common_msgs.srv


class Oracle(rclpy.node.Node):

    def __init__(self):
        super().__init__('oracle')
        self._sequence_id = 0
        self._status_pub = self.create_publisher(
            ros2_example_apps_msgs.msg.Status, 'status',
            rclpy.qos.QoSProfile(depth=1))
        self._query_server = self.create_service(
            ros2_example_common_msgs.srv.Query, 'query', self._handle_query)
        self._action_server = rclpy.action.ActionServer(
            self, ros2_example_common_msgs.action.Do, 'do',
            execute_callback=self._handle_rite_action,
            goal_callback=self._handle_action_request,
            cancel_callback=self._handle_cancelled_action,
        )
        self._status_timer = self.create_timer(1.0, self._publish_status)

    def _handle_action_request(self, goal):
        if goal.action != 'rite':
            self.get_logger().warning(
                "Don't know how to " + goal.action)
            return rclpy.action.GoalResponse.REJECT
        return rclpy.action.GoalResponse.ACCEPT

    def _handle_cancelled_action(self, handle):
        return rclpy.action.CancelResponse.ACCEPT

    def _handle_rite_action(self, handle):
        result = ros2_example_common_msgs.action.Do.Result()
        timeout = rclpy.duration.Duration.from_msg(
            handle.request.timeout)
        period = rclpy.duration.Duration.from_msg(
            handle.request.period)
        period = period.nanoseconds / 1e9
        start_time = self.get_clock().now()
        rate = self.create_rate(1 / period)
        while rclpy.ok():
            if handle.is_cancel_requested:
                handle.canceled()
                break
            current_time = self.get_clock().now()
            if current_time - start_time > timeout:
                result.reason = 'timeout'
                handle.abort()
                break
            if bool(random.getrandbits(1)):
                handle.succeed()
                break
            feedback = ros2_example_common_msgs.action.Do.Feedback()
            feedback.message = 'chanting'
            handle.publish_feedback(feedback)
            rate.sleep()
        return result

    def _handle_query(self, request, response):
        if request.query == "how's it going?":
            response.reply = 'all good!'
        else:
            response.reply = "don't know"
        return response

    def _publish_status(self):
        msg = ros2_example_apps_msgs.msg.Status()
        msg.status.sequence_id = self._sequence_id
        self._sequence_id = self._sequence_id + 1
        msg.status.message = 'OK'
        msg.origin = 'oracle'
        self._status_pub.publish(msg)


def main():
    rclpy.init()

    try:
        executor = rclpy.executors.MultiThreadedExecutor()
        rclpy.spin(Oracle(), executor=executor)
    except KeyboardInterrupt:
        pass
    finally:
        rclpy.shutdown()


if __name__ == '__main__':
    main()
