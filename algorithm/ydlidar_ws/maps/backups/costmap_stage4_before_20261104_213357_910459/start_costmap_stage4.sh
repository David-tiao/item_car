#!/usr/bin/env bash
set -Eeo pipefail
S4="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
S3="$(dirname "$S4")/keepout_stage"
ROOT="$(dirname "$S4")"
source /opt/ros/humble/setup.bash
source "$S4/ws/install/setup.bash"
exec 9>"$S4/.stage4.lock"
flock -n 9 || { echo '第四阶段已经运行；请先关闭上一窗口。'; exit 1; }
command -v rviz2 >/dev/null || { echo '缺少 rviz2'; exit 1; }
for f in "$ROOT/competition_navigation_map.yaml" "$S3/competition_keepout_nav2.yaml" "$S4/costmap_params.yaml"; do
  [[ -r "$f" ]] || { echo "找不到 $f"; exit 1; }
done
# 独立运行，不接管旧程序的进程；冲突时拒绝启动。
python3 - <<'PY'
import rclpy
from rclpy.node import Node
rclpy.init()
n=Node('competition_stage4_precheck')
try:
    for _ in range(5): rclpy.spin_once(n,timeout_sec=0.2)
    names={f'{ns.rstrip("/")}/{name}' for name,ns in n.get_node_names_and_namespaces()}
    for name in ('/global_costmap/global_costmap','/local_costmap/local_costmap',
                 '/competition_s4_main_map','/competition_s4_mask','/competition_s4_info'):
        if name in names: raise RuntimeError('已有同名节点：'+name)
    for topic in ('/map','/keepout_filter_mask','/costmap_filter_info',
                  '/global_costmap/costmap','/local_costmap/costmap'):
        pubs=n.get_publishers_info_by_topic(topic)
        if pubs: raise RuntimeError(f'{topic} 已被其他程序发布；先关闭旧地图、Keepout、GMapping 或 Nav2')
finally:
    n.destroy_node();rclpy.shutdown()
PY
LOG="$S4/logs/$(date +%Y%m%d_%H%M%S)_$$"
mkdir -p "$LOG"
PIDS=()
cleanup() {
  trap - EXIT INT TERM HUP
  for pid in "${PIDS[@]}"; do kill "$pid" 2>/dev/null || true; done
  for pid in "${PIDS[@]}"; do wait "$pid" 2>/dev/null || true; done
  echo "阶段4本次进程已清理。日志：$LOG"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP
MAPEXE="$(ros2 pkg prefix nav2_map_server)/lib/nav2_map_server/map_server"
INFOEXE="$(ros2 pkg prefix nav2_map_server)/lib/nav2_map_server/costmap_filter_info_server"
"$MAPEXE" --ros-args -r __node:=competition_s4_main_map \
  -p "yaml_filename:=$ROOT/competition_navigation_map.yaml" -p topic_name:=/map -p frame_id:=map \
  > "$LOG/main_map.log" 2>&1 & PIDS+=("$!")
"$MAPEXE" --ros-args -r __node:=competition_s4_mask \
  -p "yaml_filename:=$S3/competition_keepout_nav2.yaml" -p topic_name:=/keepout_filter_mask -p frame_id:=map \
  > "$LOG/mask.log" 2>&1 & PIDS+=("$!")
"$INFOEXE" --ros-args -r __node:=competition_s4_info \
  -p type:=0 -p filter_info_topic:=/costmap_filter_info -p mask_topic:=/keepout_filter_mask \
  -p base:=0.0 -p multiplier:=1.0 > "$LOG/filter_info.log" 2>&1 & PIDS+=("$!")
python3 "$S3/publish_start_zones.py" > "$LOG/blue_zones.log" 2>&1 & PIDS+=("$!")
python3 "$S4/test_pose_tf.py" > "$LOG/test_tf.log" 2>&1 & PIDS+=("$!")
activate() {
  local node="$1" state='' k
  for k in $(seq 1 22); do
    state="$(timeout 3 ros2 lifecycle get "$node" 2>/dev/null || true)"
    [[ -n "$state" ]] && break
    sleep 0.4
  done
  [[ -n "$state" ]] || { echo "生命周期节点没有响应：$node"; return 1; }
  if [[ "$state" == *unconfigured* ]]; then timeout 20 ros2 lifecycle set "$node" configure; fi
  state="$(timeout 6 ros2 lifecycle get "$node" 2>/dev/null || true)"
  if [[ "$state" == *inactive* ]]; then timeout 22 ros2 lifecycle set "$node" activate; fi
  state="$(timeout 6 ros2 lifecycle get "$node" 2>/dev/null || true)"
  echo "$node -> $state"
  [[ "$state" == *'active [3]'* ]]
}
for node in /competition_s4_main_map /competition_s4_mask /competition_s4_info; do
  activate "$node" || { echo "地图服务器启动失败，请查看：$LOG"; exit 1; }
done
# 验证三个服务先正常，再配置 Costmap（阶段3验收程序不依赖节点名称）。
python3 "$S3/check_ros_topics.py" || { echo "基础话题验收失败：$LOG"; exit 1; }
RUNNER="$S4/ws/install/competition_costmap_demo/lib/competition_costmap_demo/costmap_stage4_runner"
[[ -x "$RUNNER" ]] || { echo "找不到 Costmap 可执行程序：$RUNNER"; exit 1; }
"$RUNNER" --ros-args --params-file "$S4/costmap_params.yaml" \
  > "$LOG/costmaps.log" 2>&1 & PIDS+=("$!")
# Humble 标准两个独立生命周期 Costmap 节点
for node in /global_costmap/global_costmap /local_costmap/local_costmap; do
  activate "$node" || { echo "Costmap 生命周期失败。请看 $LOG/costmaps.log"; tail -n 60 "$LOG/costmaps.log" || true; exit 1; }
done
python3 "$S4/check_costmaps.py" || {
  echo 'Nav2 Costmap 验收不通过；最近日志：'
  tail -n 75 "$LOG/costmaps.log" || true
  echo "完整日志位置：$LOG"
  exit 1
}
echo '---------------------------------------------------------'
echo '已打通：/map + /keepout_filter_mask + /costmap_filter_info'
echo '全局代价图：/global_costmap/costmap'
echo '局部代价图：/local_costmap/costmap'
echo '本次 base_link 位姿固定在 (1.20,1.20)，只作 TF 演示！'
echo '不发布 /cmd_vel；不连接 STM32，不能用于实际导航。'
echo '关闭 RViz2 后清理本次启动的所有节点。'
echo "完整日志：$LOG"
echo '---------------------------------------------------------'
if [[ "${LIBGL_ALWAYS_SOFTWARE:-0}" == 1 ]]; then
  LIBGL_ALWAYS_SOFTWARE=1 rviz2 -d "$S4/stage4_preview.rviz" > "$LOG/rviz.log" 2>&1 &
else
  rviz2 -d "$S4/stage4_preview.rviz" > "$LOG/rviz.log" 2>&1 &
fi
RVIZ_PID=$!
PIDS+=("$RVIZ_PID")
wait "$RVIZ_PID" || { echo "RViz2 异常退出：$LOG/rviz.log"; exit 1; }
