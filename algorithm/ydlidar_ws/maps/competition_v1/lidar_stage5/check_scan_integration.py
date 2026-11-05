import sys
import time

import rclpy
from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener

rclpy.init()

node = Node("competition_stage5_integration_check")
buffer = Buffer()
listener = TransformListener(buffer, node)

try:
    deadline = time.monotonic() + 15

    while time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.4)

        subscribers = {
            x.node_name
            for x in node.get_subscriptions_info_by_topic("/scan")
        }

        try:
            buffer.lookup_transform("map", "laser_frame", Time())
            tf_ok = True
        except Exception:
            tf_ok = False

        if (
            {"global_costmap", "local_costmap"} <= subscribers
            and tf_ok
        ):
            break
    else:
        raise RuntimeError(
            "两个 Costmap 未同时订阅 /scan，或者 TF 不可用"
        )

    print("全局 Costmap 已订阅 /scan")
    print("局部 Costmap 已订阅 /scan")
    print("map -> laser_frame TF 正常")
    print("第五阶段接口联调通过", flush=True)

except Exception as e:
    print("联调失败：", e, file=sys.stderr)
    sys.exit(1)

finally:
    node.destroy_node()
    rclpy.shutdown()
