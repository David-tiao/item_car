#!/usr/bin/env python3
"""
工创比赛第六阶段：道路级封路与换路。

读取第五阶段已经融合 X3 雷达的 Global Costmap。
将障碍物所在栅格转换为道路封闭状态。
只输出候选路线和可视化，不控制电机。
"""

import argparse
import heapq
import json
import math
import time

COL = {'A': 0.35, 'B': 1.20, 'C': 2.05}
ROW = {'1': 0.35, '2': 1.20, '3': 2.05}

NODES = {
    c+r: (x, y)
    for c, x in COL.items()
    for r, y in ROW.items()
}

EDGES = {}

for r in ROW:
    for a, b in [('A', 'B'), ('B', 'C')]:
        EDGES[f'{a}{r}-{b}{r}'] = (
            f'{a}{r}', f'{b}{r}'
        )

for c in COL:
    for a, b in [('1', '2'), ('2', '3')]:
        EDGES[f'{c}{a}-{c}{b}'] = (
            f'{c}{a}', f'{c}{b}'
        )

# 半宽150mm + 暂定20mm裕量
# 实车仍需校核车体尺寸、转弯和定位误差
CLEARANCE = 0.17


def canonical(raw):
    raw = raw.upper().strip()

    if raw in EDGES:
        return raw

    a, b = raw.split('-') if '-' in raw else ('', '')
    reverse = f'{b}-{a}'

    if reverse in EDGES:
        return reverse

    raise ValueError('无效路段：' + raw)


def shortest(start, goal, blocked):
    queue = [(0.0, (start,))]
    best = {start: 0.0}

    while queue:
        cost, path = heapq.heappop(queue)
        here = path[-1]

        if cost > best.get(here, math.inf) + 1e-7:
            continue

        if here == goal:
            return list(path)

        for key, (a, b) in EDGES.items():
            if key in blocked or here not in (a, b):
                continue

            nxt = b if here == a else a
            nd = cost + math.dist(
                NODES[here], NODES[nxt]
            )

            if nd < best.get(nxt, math.inf) - 1e-7:
                best[nxt] = nd
                heapq.heappush(
                    queue, (nd, path + (nxt,))
                )

    return []


def index_at(msg, x, y):
    info = msg.info

    col = math.floor(
        (x - info.origin.position.x) / info.resolution
    )

    row = math.floor(
        (y - info.origin.position.y) / info.resolution
    )

    if not (
        0 <= row < info.height
        and 0 <= col < info.width
    ):
        return None

    return row * info.width + col


def scan_edges(cost, mask):
    """
    在每条道路中心线及左右安全带内取样。

    static:
        与禁行区域冲突或存在未知地图单元的路段。

    dynamic:
        Costmap 中存在实体障碍物的路段。
    """

    static = set()
    dynamic = set()

    for key, (a, b) in EDGES.items():

        (x0, y0), (x1, y1) = (
            NODES[a], NODES[b]
        )

        dx = x1 - x0
        dy = y1 - y0

        length = math.hypot(dx, dy)

        nx = -dy / length
        ny = dx / length

        fatal = set()
        unsafe = False

        n = math.ceil(length / 0.02)

        for i in range(n + 1):

            f = i / n
            x = x0 + f * dx
            y = y0 + f * dy

            for off in (
                -0.17, -0.13, -0.09,
                -0.05, 0.0,
                0.05, 0.09, 0.13, 0.17
            ):

                px = x + off * nx
                py = y + off * ny

                mi = index_at(mask, px, py)

                if mi is None or mask.data[mi] != 0:
                    unsafe = True
                    continue

                ci = index_at(cost, px, py)

                if ci is None or cost.data[ci] < 0:
                    unsafe = True

                elif cost.data[ci] >= 99:
                    fatal.add(ci)

        if unsafe:
            static.add(key)

        if len(fatal) >= 2:
            dynamic.add(key)

    return static, dynamic


def run():

    parser = argparse.ArgumentParser(
        description="田字形道路拓扑规划测试"
    )

    parser.add_argument(
        '--start',
        choices=sorted(NODES),
        default='C1'
    )

    parser.add_argument(
        '--goal',
        choices=sorted(NODES),
        default='A3'
    )

    parser.add_argument(
        '--block',
        action='append',
        default=[]
    )

    parser.add_argument(
        '--demo',
        action='store_true'
    )

    args = parser.parse_args()

    manual = {
        canonical(v)
        for v in args.block
    }

    if args.demo:
        print("道路节点：", sorted(NODES))
        print("路段总数：", len(EDGES))
        print("人工封路：", sorted(manual))

        path = shortest(
            args.start, args.goal, manual
        )

        print(
            "规划结果：",
            " -> ".join(path)
            if path else "无可通行路线"
        )
        return

    import rclpy

    from rclpy.node import Node
    from rclpy.qos import (
        QoSProfile,
        DurabilityPolicy,
        ReliabilityPolicy
    )

    from nav_msgs.msg import OccupancyGrid, Path
    from visualization_msgs.msg import Marker, MarkerArray
    from geometry_msgs.msg import PoseStamped, Point
    from std_msgs.msg import String

    class Topology(Node):

        def __init__(self):
            super().__init__(
                'competition_stage6_road_topology'
            )

            self.mask = None
            self.cost = None
            self.last_cost = 0.0

            self.manual = manual

            self.still_static = set()
            self.still_dynamic = set()

            self.found = {
                k: 0 for k in EDGES
            }

            self.cleared = {
                k: 0 for k in EDGES
            }

            qos = QoSProfile(
                depth=1,
                durability=DurabilityPolicy.TRANSIENT_LOCAL,
                reliability=ReliabilityPolicy.RELIABLE
            )

            self.sm = self.create_subscription(
                OccupancyGrid,
                '/keepout_filter_mask',
                self.got_mask,
                qos
            )

            self.sc = self.create_subscription(
                OccupancyGrid,
                '/global_costmap/costmap',
                self.got_cost,
                qos
            )

            self.pub_graph = self.create_publisher(
                MarkerArray,
                '/competition/road_graph',
                qos
            )

            self.pub_path = self.create_publisher(
                Path,
                '/competition/road_path',
                qos
            )

            self.pub_status = self.create_publisher(
                String,
                '/competition/road_status',
                qos
            )

            self.timer = self.create_timer(
                1.0,
                self.publish
            )

            self.get_logger().warning(
                "当前只规划和显示路线，不发送电机指令！"
            )

        def got_mask(self, msg):
            self.mask = msg

        def got_cost(self, msg):

            self.cost = msg
            self.last_cost = time.monotonic()

            if self.mask is None:
                return

            sta, dyn = scan_edges(
                msg, self.mask
            )

            self.still_static = sta

            for key in EDGES:

                if key in dyn:
                    self.found[key] += 1
                    self.cleared[key] = 0

                else:
                    self.cleared[key] += 1
                    self.found[key] = 0

                # 连续两次受阻：封路
                if self.found[key] >= 2:
                    self.still_dynamic.add(key)

                # 连续三次清空：恢复
                if self.cleared[key] >= 3:
                    self.still_dynamic.discard(key)

        def marker(
            self, ident, ns, typ, color,
            points=(), pos=None, label='', size=.04
        ):

            m = Marker()

            m.header.frame_id = 'map'
            m.header.stamp = self.get_clock().now().to_msg()

            m.ns = ns
            m.id = ident
            m.type = typ
            m.action = Marker.ADD

            m.pose.orientation.w = 1.0

            m.color.r = color[0]
            m.color.g = color[1]
            m.color.b = color[2]
            m.color.a = 1.0

            m.scale.x = size

            if typ == Marker.LINE_STRIP:

                m.points = [
                    Point(
                        x=float(x),
                        y=float(y),
                        z=0.055
                    )
                    for x, y in points
                ]

            elif pos is not None:

                m.pose.position.x = pos[0]
                m.pose.position.y = pos[1]

                if typ == Marker.TEXT_VIEW_FACING:
                    m.pose.position.z = 0.12
                    m.text = label
                    m.scale.z = 0.095

                else:
                    m.pose.position.z = 0.06
                    m.scale.y = size
                    m.scale.z = size

            return m

        def publish(self):

            ready = (
                self.mask is not None
                and self.cost is not None
                and time.monotonic() - self.last_cost < 5.0
            )

            blocked = (
                self.manual
                | self.still_static
                | self.still_dynamic
            )

            path = shortest(
                args.start,
                args.goal,
                blocked
            ) if ready else []

            pm = Path()

            pm.header.frame_id = 'map'
            pm.header.stamp = self.get_clock().now().to_msg()

            for name in path:

                p = PoseStamped()
                p.header = pm.header

                p.pose.position.x = NODES[name][0]
                p.pose.position.y = NODES[name][1]
                p.pose.orientation.w = 1.0

                pm.poses.append(p)

            self.pub_path.publish(pm)

            markers = MarkerArray()

            for i, (key, (a, b)) in enumerate(
                EDGES.items()
            ):

                if key in self.manual | self.still_dynamic:
                    color = (1.0, 0.15, 0.15)

                elif key in self.still_static:
                    color = (1.0, 0.55, 0.0)

                else:
                    color = (0.12, 0.78, 0.2)

                markers.markers.append(
                    self.marker(
                        i,
                        'roads',
                        Marker.LINE_STRIP,
                        color,
                        [NODES[a], NODES[b]],
                        size=0.03
                    )
                )

            for i, key in enumerate(sorted(NODES)):

                markers.markers.append(
                    self.marker(
                        i, 'dots',
                        Marker.SPHERE,
                        (0.1, 0.55, 1.0),
                        pos=NODES[key],
                        size=0.055
                    )
                )

                markers.markers.append(
                    self.marker(
                        i, 'text',
                        Marker.TEXT_VIEW_FACING,
                        (1.0, 1.0, 1.0),
                        pos=NODES[key],
                        label=key
                    )
                )

            markers.markers.append(
                self.marker(
                    0,
                    'chosen',
                    Marker.LINE_STRIP,
                    (0.0, 0.9, 1.0),
                    [NODES[x] for x in path],
                    size=0.055
                )
            )

            self.pub_graph.publish(markers)

            result = {
                'ready': ready,
                'start': args.start,
                'goal': args.goal,
                'blocked': sorted(blocked),
                'blocked_by_lidar_costmap': sorted(
                    self.still_dynamic
                ),
                'route': path,
                'status': (
                    'WAIT_FOR_COSTMAP'
                    if not ready
                    else 'OK' if path
                    else 'NO_ROUTE'
                )
            }

            self.pub_status.publish(
                String(
                    data=json.dumps(
                        result,
                        ensure_ascii=False
                    )
                )
            )

            if ready:

                self.get_logger().info(
                    '封路=' + str(sorted(blocked))
                    + ' | 路线='
                    + (
                        '→'.join(path)
                        if path
                        else '无可走路线'
                    )
                )

            else:
                self.get_logger().warning(
                    '等待 Keepout Mask 和 Global Costmap'
                )

    rclpy.init()
    node = Topology()

    try:
        rclpy.spin(node)

    except KeyboardInterrupt:
        pass

    finally:
        node.destroy_node()

        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    run()
