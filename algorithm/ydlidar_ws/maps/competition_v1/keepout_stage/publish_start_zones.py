#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from visualization_msgs.msg import Marker, MarkerArray
from rclpy.qos import QoSProfile, DurabilityPolicy, ReliabilityPolicy

ZONES=[(2.10,2.10,2.40,2.40),(2.10,0.00,2.40,0.30)]
class StartZones(Node):
    def __init__(self):
        super().__init__('competition_keepout_blue_start_zones')
        qos=QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL,
                       reliability=ReliabilityPolicy.RELIABLE)
        self.pub=self.create_publisher(MarkerArray, '/competition_start_zones', qos)
        self.timer=self.create_timer(1.0,self.send)
        self.send()
    def send(self):
        arr=MarkerArray()
        stamp=self.get_clock().now().to_msg()
        for idx,(x0,y0,x1,y1) in enumerate(ZONES,1):
            marker=Marker()
            marker.header.frame_id='map'; marker.header.stamp=stamp
            marker.ns='start_zones'; marker.id=idx; marker.action=Marker.ADD; marker.type=Marker.CUBE
            marker.pose.position.x=(x0+x1)/2; marker.pose.position.y=(y0+y1)/2
            marker.pose.position.z=0.025; marker.pose.orientation.w=1.0
            marker.scale.x=x1-x0; marker.scale.y=y1-y0; marker.scale.z=0.03
            marker.color.r=0.06; marker.color.g=0.24; marker.color.b=1.0; marker.color.a=0.9
            arr.markers.append(marker)
        self.pub.publish(arr)
rclpy.init()
node=StartZones()
try:
    rclpy.spin(node)
except KeyboardInterrupt:
    pass
finally:
    node.destroy_node()
    if rclpy.ok(): rclpy.shutdown()
