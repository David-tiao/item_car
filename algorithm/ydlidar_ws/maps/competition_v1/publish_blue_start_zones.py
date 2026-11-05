#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, DurabilityPolicy
from visualization_msgs.msg import Marker, MarkerArray

ZONES = [
    (2.10, 2.10, 2.40, 2.40),
    (2.10, 0.00, 2.40, 0.30),
]

class StartZones(Node):
    def __init__(self):
        super().__init__("competition_blue_start_zones")
        qos = QoSProfile(depth=1)
        qos.durability = DurabilityPolicy.TRANSIENT_LOCAL
        self.pub = self.create_publisher(
            MarkerArray, "/competition_start_zones", qos
        )
        self.timer = self.create_timer(1.0, self.publish_zones)
        self.publish_zones()

    def publish_zones(self):
        result = MarkerArray()
        stamp = self.get_clock().now().to_msg()

        for i, (x0, y0, x1, y1) in enumerate(ZONES, 1):
            m = Marker()
            m.header.frame_id = "map"
            m.header.stamp = stamp
            m.ns = "start_area"
            m.id = i
            m.type = Marker.CUBE
            m.action = Marker.ADD

            m.pose.position.x = (x0 + x1) / 2
            m.pose.position.y = (y0 + y1) / 2
            m.pose.position.z = 0.025
            m.pose.orientation.w = 1.0

            m.scale.x = x1 - x0
            m.scale.y = y1 - y0
            m.scale.z = 0.03

            m.color.r = 0.05
            m.color.g = 0.25
            m.color.b = 1.0
            m.color.a = 0.94
            result.markers.append(m)

            label = Marker()
            label.header.frame_id = "map"
            label.header.stamp = stamp
            label.ns = "start_label"
            label.id = i
            label.type = Marker.TEXT_VIEW_FACING
            label.action = Marker.ADD
            label.pose.position.x = (x0 + x1) / 2
            label.pose.position.y = (y0 + y1) / 2
            label.pose.position.z = 0.09
            label.pose.orientation.w = 1.0
            label.scale.z = 0.075
            label.color.r = 1.0
            label.color.g = 1.0
            label.color.b = 1.0
            label.color.a = 1.0
            label.text = f"START {i}"
            result.markers.append(label)

        self.pub.publish(result)

rclpy.init()
node = StartZones()
try:
    rclpy.spin(node)
except KeyboardInterrupt:
    pass
finally:
    node.destroy_node()
    if rclpy.ok():
        rclpy.shutdown()
