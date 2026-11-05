import math
import time
import sys

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import LaserScan

rclpy.init()
node = Node("competition_stage5_scan_check")
msgs = []

sub = node.create_subscription(
    LaserScan,
    "/scan",
    lambda m: msgs.append(m),
    qos_profile_sensor_data
)

try:
    deadline = time.monotonic() + 18

    while time.monotonic() < deadline and not msgs:
        rclpy.spin_once(node, timeout_sec=0.3)

    if not msgs:
        raise RuntimeError("18秒内没有收到 /scan")

    msg = msgs[0]

    valid = sum(
        math.isfinite(x) and msg.range_min <= x <= msg.range_max
        for x in msg.ranges
    )

    print(
        f"/scan 正常：frame={msg.header.frame_id} "
        f"点数={len(msg.ranges)} 有效点={valid}",
        flush=True
    )

    if msg.header.frame_id != "laser_frame" or valid < 3:
        raise RuntimeError("扫描坐标系或有效点数异常")

except Exception as e:
    print("雷达检查失败：", e, file=sys.stderr)
    sys.exit(1)

finally:
    node.destroy_node()
    rclpy.shutdown()
