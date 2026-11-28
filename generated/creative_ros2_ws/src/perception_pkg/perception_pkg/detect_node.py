"""假感知：周期性发布一个目标位姿（带 stamp 语义用 header）。"""
from __future__ import annotations

import math

import rclpy
from geometry_msgs.msg import PoseArray, Pose
from rclpy.node import Node
from std_msgs.msg import Header


class DetectNode(Node):
    def __init__(self) -> None:
        super().__init__("detect_node")
        self._pub = self.create_publisher(PoseArray, "/perception/targets", 10)
        self._t = 0.0
        self.create_timer(0.1, self._tick)

    def _tick(self) -> None:
        self._t += 0.1
        msg = PoseArray()
        msg.header = Header()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "map"
        p = Pose()
        p.position.x = 1.0 + 0.2 * math.sin(self._t)
        p.position.y = 0.0
        msg.poses = [p]
        self._pub.publish(msg)


def main() -> None:
    rclpy.init()
    node = DetectNode()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
