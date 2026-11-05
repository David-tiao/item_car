#!/usr/bin/env bash
# 这一步仅启动地图、掩膜、过滤器信息和 RViz2，不启动 Nav2 控制器或电机。
set -Eeo pipefail
STAGE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$STAGE")"
source /opt/ros/humble/setup.bash

exec 9>"$STAGE/.keepout.lock"
flock -n 9 || { echo 'Keepout 已经运行；请勿重复双击。'; exit 1; }
for pkg in nav2_map_server nav2_msgs nav_msgs visualization_msgs; do
  ros2 pkg prefix "$pkg" >/dev/null 2>&1 || {
    echo "缺少 ROS 2 包 $pkg，请检查 Humble / 安装 ros-humble-navigation2"
    exit 1
  }
done
PREFIX="$(ros2 pkg prefix nav2_map_server)/lib/nav2_map_server"
for exe in map_server costmap_filter_info_server; do
  [[ -x "$PREFIX/$exe" ]] || { echo "缺少可执行文件：$PREFIX/$exe"; exit 1; }
done

# 不清理用户自己的程序：如果旧预览或 GMapping 正在发布 /map，先要求关闭。
python3 - <<'PY'
import rclpy
from rclpy.node import Node
rclpy.init()
node=Node('competition_keepout_preflight')
try:
    for _ in range(5): rclpy.spin_once(node,timeout_sec=0.2)
    existing = {name for name, _ in node.get_node_names_and_namespaces()}
    for name in ('competition_main_map_server', 'competition_keepout_mask_server',
                 'competition_keepout_info_server', 'competition_keepout_blue_start_zones'):
        if name in existing:
            raise SystemExit('发现残留 ROS 节点 ' + name + '；请先结束旧窗口，避免生命周期服务冲突。')
    for topic in ('/map','/keepout_filter_mask','/costmap_filter_info','/competition_start_zones'):
        pubs=node.get_publishers_info_by_topic(topic)
        if pubs:
            print('已有节点正在发布',topic, '，请先关闭旧的静态地图/GMapping/Keepout 窗口。')
            for p in pubs: print('  ',p.node_namespace,p.node_name)
            raise SystemExit(2)
finally:
    node.destroy_node();rclpy.shutdown()
PY

LOG="$STAGE/logs/$(date +%Y%m%d_%H%M%S)_$$"
mkdir -p "$LOG"
PIDS=()
cleanup() {
  trap - EXIT INT TERM HUP
  for pid in "${PIDS[@]}"; do kill "$pid" 2>/dev/null || true; done
  for pid in "${PIDS[@]}"; do wait "$pid" 2>/dev/null || true; done
  echo "本次启动的地图服务已清理。日志：$LOG"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

"$PREFIX/map_server" --ros-args -r __node:=competition_main_map_server \
  -p "yaml_filename:=$ROOT/competition_rules_preview.yaml" \
  -p topic_name:=/map -p frame_id:=map > "$LOG/main_map.log" 2>&1 & PIDS+=("$!")
"$PREFIX/map_server" --ros-args -r __node:=competition_keepout_mask_server \
  --params-file "$STAGE/keepout_servers.yaml" > "$LOG/mask_server.log" 2>&1 & PIDS+=("$!")
"$PREFIX/costmap_filter_info_server" --ros-args -r __node:=competition_keepout_info_server \
  --params-file "$STAGE/keepout_servers.yaml" > "$LOG/info_server.log" 2>&1 & PIDS+=("$!")
python3 "$STAGE/publish_start_zones.py" > "$LOG/blue_start_zones.log" 2>&1 & PIDS+=("$!")

activate_node() {
  local name="$1" state="" j
  for j in $(seq 1 25); do
    state=$(timeout 4 ros2 lifecycle get "$name" 2>/dev/null || true)
    [[ -n "$state" ]] && break
    sleep 0.4
  done
  [[ -n "$state" ]] || { echo "节点 $name 未启动；请看日志 $LOG"; return 1; }
  if [[ "$state" == *unconfigured* ]]; then
    timeout 12 ros2 lifecycle set "$name" configure
  fi
  state=$(timeout 5 ros2 lifecycle get "$name" 2>/dev/null || true)
  if [[ "$state" == *inactive* ]]; then
    timeout 12 ros2 lifecycle set "$name" activate
  fi
  state=$(timeout 5 ros2 lifecycle get "$name" 2>/dev/null || true)
  echo "$name -> $state"
  [[ "$state" == *'active [3]'* ]] || return 1
}

for node in /competition_main_map_server /competition_keepout_mask_server /competition_keepout_info_server; do
  activate_node "$node" || { echo '生命周期激活失败，日志：' "$LOG"; exit 1; }
done

python3 "$STAGE/check_ros_topics.py" || { echo "话题验收失败，日志：$LOG"; exit 1; }

echo '---------------------------------------------'
echo '静态地图：/map'
echo '禁行掩膜：/keepout_filter_mask'
echo 'Filter 信息：/costmap_filter_info  (type=0)'
echo 'RViz 蓝色启停区：/competition_start_zones'
echo '退出 RViz2 会自动关闭本次启动的节点。'
echo "日志：$LOG"
echo '---------------------------------------------'

if [[ "${LIBGL_ALWAYS_SOFTWARE:-0}" == 1 ]]; then
  LIBGL_ALWAYS_SOFTWARE=1 rviz2 -d "$STAGE/keepout_preview.rviz" > "$LOG/rviz2.log" 2>&1 &
else
  rviz2 -d "$STAGE/keepout_preview.rviz" > "$LOG/rviz2.log" 2>&1 &
fi
PIDS+=("$!")
wait "${PIDS[-1]}" || { echo "RViz2 异常退出，请查看 $LOG/rviz2.log"; exit 1; }
