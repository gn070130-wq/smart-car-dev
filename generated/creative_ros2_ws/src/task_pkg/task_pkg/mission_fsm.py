"""任务机：IDLE → GOTO_TARGET → DONE；超时回 IDLE。"""
from __future__ import annotations

import rclpy
from geometry_msgs.msg import PoseArray, PoseStamped
from rclpy.node import Node
from std_msgs.msg import String


class MissionFsm(Node):
    def __init__(self) -> None:
        super().__init__("mission_fsm")
        self._state = "IDLE"
        self._goal_pub = self.create_publisher(PoseStamped, "/goal_pose", 10)
        self._state_pub = self.create_publisher(String, "/mission/state", 10)
        self.create_subscription(PoseArray, "/perception/targets", self._on_targets, 10)
        self._last_target = None
        self._ticks_in_state = 0
        self.create_timer(0.1, self._tick)

    def _on_targets(self, msg: PoseArray) -> None:
        if msg.poses:
            self._last_target = msg.poses[0]

    def _publish_state(self) -> None:
        s = String()
        s.data = self._state
        self._state_pub.publish(s)

    def _tick(self) -> None:
        self._ticks_in_state += 1
        if self._state == "IDLE":
            if self._last_target is not None:
                self._state = "GOTO_TARGET"
                self._ticks_in_state = 0
        elif self._state == "GOTO_TARGET":
            g = PoseStamped()
            g.header.stamp = self.get_clock().now().to_msg()
            g.header.frame_id = "map"
            g.pose = self._last_target
            self._goal_pub.publish(g)
            if self._ticks_in_state > 50:  # ~5s 示意完成
                self._state = "DONE"
                self._ticks_in_state = 0
        elif self._state == "DONE":
            if self._ticks_in_state > 20:
                self._state = "IDLE"
                self._last_target = None
                self._ticks_in_state = 0
        if self._ticks_in_state > 200:  # 全局超时
            self._state = "IDLE"
            self._ticks_in_state = 0
        self._publish_state()


def main() -> None:
    rclpy.init()
    node = MissionFsm()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
