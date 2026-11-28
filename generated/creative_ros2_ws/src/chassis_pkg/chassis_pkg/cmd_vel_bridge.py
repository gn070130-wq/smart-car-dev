"""cmd_vel 看门狗：超时停转；此处仅日志（真机改串口/CAN）。"""
from __future__ import annotations

import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node


class CmdVelBridge(Node):
    def __init__(self) -> None:
        super().__init__("cmd_vel_bridge")
        self.create_subscription(Twist, "/cmd_vel", self._on_cmd, 10)
        self._last_ms = self.get_clock().now().nanoseconds // 1_000_000
        self._vx = 0.0
        self._w = 0.0
        self.create_timer(0.05, self._watchdog)

    def _on_cmd(self, msg: Twist) -> None:
        self._vx = msg.linear.x
        self._w = msg.angular.z
        self._last_ms = self.get_clock().now().nanoseconds // 1_000_000

    def _watchdog(self) -> None:
        now = self.get_clock().now().nanoseconds // 1_000_000
        if now - self._last_ms > 200:
            self._vx = 0.0
            self._w = 0.0
        # 真机：写串口；host：低频打印
        if int(now / 500) % 2 == 0:
            self.get_logger().debug(f"bridge vx={self._vx:.2f} w={self._w:.2f}")


def main() -> None:
    rclpy.init()
    node = CmdVelBridge()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
