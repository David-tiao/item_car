import time
import rclpy

from rclpy.node import Node
from rclpy.qos import (
    QoSProfile,
    DurabilityPolicy,
    ReliabilityPolicy
)
from nav_msgs.msg import OccupancyGrid

rclpy.init()
node = Node("competition_stage5_obstacle_monitor")

qos = QoSProfile(
    depth=1,
    durability=DurabilityPolicy.TRANSIENT_LOCAL,
    reliability=ReliabilityPolicy.RELIABLE
)

msgs = {}

topics = [
    "/keepout_filter_mask",
    "/local_costmap/costmap"
]

subs = [
    node.create_subscription(
        OccupancyGrid,
        t,
        lambda m, t=t: msgs.__setitem__(t, m),
        qos
    )
    for t in topics
]

def value_at(msg, x, y):
    info = msg.info

    col = int(
        (x - info.origin.position.x) / info.resolution
    )
    row = int(
        (y - info.origin.position.y) / info.resolution
    )

    if 0 <= col < info.width and 0 <= row < info.height:
        return msg.data[row * info.width + col]

    return 100

try:
    print("正在监控局部代价地图...", flush=True)
    print("放入或移走障碍物，观察数值变化。", flush=True)

    last = 0

    while True:
        rclpy.spin_once(node, timeout_sec=0.3)

        if time.monotonic() - last < 2:
            continue

        last = time.monotonic()

        if len(msgs) < 2:
            print("等待地图数据...", flush=True)
            continue

        cost = msgs["/local_costmap/costmap"]
        mask = msgs["/keepout_filter_mask"]
        info = cost.info

        count = 0

        for row in range(info.height):
            y = (
                info.origin.position.y
                + (row + 0.5) * info.resolution
            )

            if not 0 <= y < 2.4:
                continue

            for col in range(info.width):
                x = (
                    info.origin.position.x
                    + (col + 0.5) * info.resolution
                )

                if (
                    0 <= x < 2.4
                    and cost.data[row * info.width + col] >= 99
                    and value_at(mask, x, y) == 0
                ):
                    count += 1

        print(
            f"允许区域内的致命占据栅格数：{count}",
            flush=True
        )

except KeyboardInterrupt:
    pass

finally:
    node.destroy_node()
    rclpy.shutdown()
