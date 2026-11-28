from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
    return LaunchDescription(
        [
            Node(package="perception_pkg", executable="detect_node", name="detect_node"),
            Node(package="task_pkg", executable="mission_fsm", name="mission_fsm"),
            Node(package="nav_pkg", executable="simple_nav", name="simple_nav"),
            Node(package="chassis_pkg", executable="cmd_vel_bridge", name="cmd_vel_bridge"),
        ]
    )
