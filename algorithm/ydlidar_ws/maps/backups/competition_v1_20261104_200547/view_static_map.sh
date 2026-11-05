#!/usr/bin/env bash
set -Eeo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source /opt/ros/humble/setup.bash

# 防止重复双击启动
exec 9>"$ROOT/.map.lock"
flock -n 9 || {
    echo "静态地图已经运行，请不要重复启动"
    exit 1
}

if [[ "${1:-}" == "--physical" ]]; then
    MAP="$ROOT/competition_navigation_map.yaml"
else
    MAP="$ROOT/competition_rules_preview.yaml"
fi

LOG="$ROOT/logs/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$LOG"

MAP_PID=""
RVIZ_PID=""

cleanup() {
    trap - EXIT INT TERM HUP

    [[ -z "$RVIZ_PID" ]] || kill "$RVIZ_PID" 2>/dev/null || true
    [[ -z "$MAP_PID" ]] || kill "$MAP_PID" 2>/dev/null || true

    [[ -z "$RVIZ_PID" ]] || wait "$RVIZ_PID" 2>/dev/null || true
    [[ -z "$MAP_PID" ]] || wait "$MAP_PID" 2>/dev/null || true
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

# 拒绝与已经运行的 GMapping 或其他地图服务器冲突
python3 - <<'PY'
import rclpy
from rclpy.node import Node

rclpy.init()
node = Node("competition_map_precheck")

try:
    for _ in range(6):
        rclpy.spin_once(node, timeout_sec=0.3)

    publishers = node.get_publishers_info_by_topic("/map")

    if publishers:
        print("错误：/map 已有其他发布者，请先停止旧地图服务或 GMapping")
        for p in publishers:
            print(p.node_namespace, p.node_name)
        raise SystemExit(2)
finally:
    node.destroy_node()
    rclpy.shutdown()
PY

EXE="$(ros2 pkg prefix nav2_map_server)/lib/nav2_map_server/map_server"

"$EXE" \
  --ros-args \
  -r __node:=competition_static_map_server \
  -p "yaml_filename:=$MAP" \
  -p topic_name:=map \
  -p frame_id:=map \
  > "$LOG/map_server.log" 2>&1 &

MAP_PID=$!

echo "正在等待 Map Server..."

READY=0

for i in $(seq 1 25); do
    if ! kill -0 "$MAP_PID" 2>/dev/null; then
        echo "Map Server 提前退出"
        tail -n 40 "$LOG/map_server.log"
        exit 1
    fi

    if timeout 3 ros2 lifecycle get \
      /competition_static_map_server >/dev/null 2>&1; then
        READY=1
        break
    fi

    sleep 0.4
done

if [[ "$READY" != 1 ]]; then
    echo "Map Server 生命周期服务超时"
    tail -n 40 "$LOG/map_server.log"
    exit 1
fi

STATE=$(timeout 6 ros2 lifecycle get /competition_static_map_server)

if [[ "$STATE" == *unconfigured* ]]; then
    timeout 10 ros2 lifecycle set \
      /competition_static_map_server configure
fi

STATE=$(timeout 6 ros2 lifecycle get /competition_static_map_server)

if [[ "$STATE" == *inactive* ]]; then
    timeout 10 ros2 lifecycle set \
      /competition_static_map_server activate
fi

STATE=$(timeout 6 ros2 lifecycle get /competition_static_map_server)

echo "地图生命周期：$STATE"

if [[ "$STATE" != *"active [3]"* ]]; then
    echo "地图激活失败"
    tail -n 40 "$LOG/map_server.log"
    exit 1
fi

# 检查是否真正接收到地图，不仅仅检查话题是否存在
python3 - <<'PY'
import time
import rclpy

from nav_msgs.msg import OccupancyGrid
from rclpy.node import Node
from rclpy.qos import (
    QoSProfile,
    DurabilityPolicy,
    ReliabilityPolicy
)

rclpy.init()

node = Node("competition_map_checker")

received = []

qos = QoSProfile(
    depth=1,
    durability=DurabilityPolicy.TRANSIENT_LOCAL,
    reliability=ReliabilityPolicy.RELIABLE
)

sub = node.create_subscription(
    OccupancyGrid,
    "/map",
    lambda msg: received.append(msg),
    qos
)

try:
    deadline = time.monotonic() + 12

    while time.monotonic() < deadline and not received:
        rclpy.spin_once(node, timeout_sec=0.3)

    publishers = node.get_publishers_info_by_topic("/map")

    print("地图发布者数量：", len(publishers))

    if len(publishers) != 1:
        raise SystemExit("错误：/map 不是唯一发布者")

    if not received:
        raise SystemExit("错误：没有收到 /map 地图数据")

    msg = received[0]

    print("地图尺寸：", msg.info.width, msg.info.height)
    print("地图分辨率：", msg.info.resolution)

    if msg.info.width != 280 or msg.info.height != 280:
        raise SystemExit("错误：地图尺寸不正确")

    if msg.header.frame_id != "map":
        raise SystemExit("错误：地图坐标系不正确")
finally:
    node.destroy_node()
    rclpy.shutdown()
PY

echo "地图服务器已经正常启动"

# RViz2 自动加载地图显示配置
if [[ "${LIBGL_ALWAYS_SOFTWARE:-0}" == 1 ]]; then
    LIBGL_ALWAYS_SOFTWARE=1 rviz2 \
      -d "$ROOT/preview_map.rviz" \
      > "$LOG/rviz2.log" 2>&1 &
else
    rviz2 \
      -d "$ROOT/preview_map.rviz" \
      > "$LOG/rviz2.log" 2>&1 &
fi

RVIZ_PID=$!

echo "===================================="
echo "静态地图 V3 启动成功"
echo "Map Server: active"
echo "Map Topic: /map"
echo "Fixed Frame: map"
echo "日志目录: $LOG"
echo "关闭 RViz2 将自动清理地图服务器"
echo "===================================="

wait "$RVIZ_PID" || {
    echo "RViz2 异常退出，检查：$LOG/rviz2.log"
    exit 1
}
