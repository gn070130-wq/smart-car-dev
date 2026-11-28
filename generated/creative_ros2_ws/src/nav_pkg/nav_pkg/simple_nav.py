"""goal_pose → cmd_vel（假设车在原点，仅 P 控制示意）。"""
from __future__ import annotations

import rclpy
from geometry_msgs.msg import PoseStamped, Twist
from rclpy.node import Node


class SimpleNav(Node):
    def __init__(self) -> None:
        super().__init__("simple_nav")
        self._cmd = self.create_publisher(Twist, "/cmd_vel", 10)
        self.create_subscription(PoseStamped, "/goal_pose", self._on_goal, 10)
        self._gx = 0.0
        self._gy = 0.0
        self._have = False
        self.create_timer(0.05, self._tick)

    def _on_goal(self, msg: PoseStamped) -> None:
        self._gx = msg.pose.position.x
        self._gy = msg.pose.position.y
        self._have = True

    def _tick(self) -> None:
        tw = Twist()
        if self._have:
            # 假定位于 (0,0)：朝 goal 前进
            tw.linear.x = max(-0.4, min(0.4, 0.5 * self._gx))
            tw.angular.z = max(-0.8, min(0.8, 1.0 * self._gy))
        self._cmd.publish(tw)


def main() -> None:
    rclpy.init()
    node = SimpleNav()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
