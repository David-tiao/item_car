#!/usr/bin/env python3
"""只用于第四阶段无底盘演示：静态机器人位姿；不可用于实际导航！"""
import rclpy
from rclpy.node import Node
from tf2_ros import StaticTransformBroadcaster
from geometry_msgs.msg import TransformStamped

rclpy.init()
node=Node('competition_stage4_TEST_POSE_not_for_driving')
br=StaticTransformBroadcaster(node)
trans=[]
for parent, child, x, y in [('map','odom',0.0,0.0), ('odom','base_link',1.20,1.20)]:
    t=TransformStamped()
    t.header.stamp=node.get_clock().now().to_msg()
    t.header.frame_id=parent
    t.child_frame_id=child
    t.transform.translation.x=x
    t.transform.translation.y=y
    t.transform.rotation.w=1.0
    trans.append(t)
br.sendTransform(trans)
node.get_logger().warning('TEST TF ONLY: base_link 固定(1.20,1.20)，未接入真实里程计，严禁驱动小车')
try:
    rclpy.spin(node)
except KeyboardInterrupt:
    pass
finally:
    node.destroy_node()
    if rclpy.ok(): rclpy.shutdown()
