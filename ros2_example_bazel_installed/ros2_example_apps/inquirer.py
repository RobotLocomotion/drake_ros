#!/usr/bin/env python3

import action_msgs.msg
import rclpy
import rclpy.action
import rclpy.duration
import rclpy.node
import rclpy.qos

import ros2_example_apps_msgs.msg
import ros2_example_common_msgs.action
import ros2_example_common_msgs.srv


class Inquirer(rclpy.node.Node):

    def __init__(self):
        super().__init__('inquirer')
        self._status_sub = self.create_subscription(
            ros2_example_apps_msgs.msg.Status, 'status',
            self._on_status, rclpy.qos.QoSProfile(depth=1))
        self._query_client = self.create_client(
            ros2_example_common_msgs.srv.Query, 'query')
        self._action_client = rclpy.action.ActionClient(
            self, ros2_example_common_msgs.action.Do, 'do')
        self._inquire_timer = self.create_timer(5.0, self.inquire)

    def _on_status(self, msg):
        self.get_logger().info('{} status ({}): {}'.format(
            msg.origin, msg.status.sequence_id, msg.status.message
        ))

    def _handle_reply(self, future):
        self.get_logger().info('oracle said: ' + future.result().reply)

    def _handle_rite_request_response(self, future):
        handle = future.result()
        if not handle.accepted:
            self.get_logger().error('oracle rejected rite request')
            return
        self.get_logger().info('oracle rite in progress')
        future = handle.get_result_async()
        future.add_done_callback(self._handle_rite_result)

    def _handle_rite_feedback(self, msg):
        self.get_logger().info('oracle is ' + msg.feedback.message)

    def _handle_rite_result(self, future):
        result = future.result()
        if result.status == action_msgs.msg.GoalStatus.STATUS_SUCCEEDED:
            self.get_logger().info('oracle rite is complete')
        elif result.status == action_msgs.msg.GoalStatus.STATUS_ABORTED:
            self.get_logger().error(
                'oracle rite aborted due to ' + result.result.reason
            )
        elif result.status == action_msgs.msg.GoalStatus.STATUS_CANCELED:
            self.get_logger().error('oracle rite was cancelled')
        else:
            self.get_logger().error('oracle rite status unknown')

    def inquire(self):
        if self._query_client.service_is_ready():
            request = ros2_example_common_msgs.srv.Query.Request()
            request.query = "how's it going?"
            self.get_logger().info('oracle, ' + request.query)
            future = self._query_client.call_async(request)
            future.add_done_callback(self._handle_reply)
        else:
            self.get_logger().warning('oracle not available for queries')
        if self._action_client.server_is_ready():
            goal = ros2_example_common_msgs.action.Do.Goal()
            goal.action = 'rite'
            goal.period = rclpy.duration.Duration(seconds=0.1).to_msg()
            goal.timeout = rclpy.duration.Duration(seconds=1.0).to_msg()
            future = self._action_client.send_goal_async(
                goal, feedback_callback=self._handle_rite_feedback)
            future.add_done_callback(self._handle_rite_request_response)
        else:
            self.get_logger().warning('oracle not available for actions')


def main():
    rclpy.init()

    try:
        rclpy.spin(Inquirer())
    except KeyboardInterrupt:
        pass
    finally:
        rclpy.shutdown()


if __name__ == '__main__':
    main()
