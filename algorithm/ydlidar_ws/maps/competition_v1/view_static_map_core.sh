#!/usr/bin/env bash
set -Eeo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source /opt/ros/humble/setup.bash

MAP="$ROOT/competition_rules_preview.yaml"
if [[ "${1:-}" == "--physical" ]]; then
    MAP="$ROOT/competition_navigation_map.yaml"
elif [[ "${1:-}" == "--keepout" ]]; then
    MAP="$ROOT/competition_keepout_mask.yaml"
fi

LOG_DIR="$ROOT/logs/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$LOG_DIR"

pkill -f 'competition_static_map_server' 2>/dev/null || true
sleep 1

ros2 run nav2_map_server map_server --ros-args \
  -r __node:=competition_static_map_server \
  -p "yaml_filename:=$MAP" \
  -p topic_name:=map \
  -p frame_id:=map \
  > "$LOG_DIR/map_server.log" 2>&1 &
MAP_PID=$!

cleanup() {
    kill "$MAP_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

for i in $(seq 1 30); do
    if ros2 lifecycle get /competition_static_map_server >/dev/null 2>&1; then
        break
    fi
    sleep 0.3
done

STATE="$(ros2 lifecycle get /competition_static_map_server 2>/dev/null || true)"

if [[ "$STATE" == *unconfigured* ]]; then
    ros2 lifecycle set /competition_static_map_server configure >/dev/null
fi

STATE="$(ros2 lifecycle get /competition_static_map_server 2>/dev/null || true)"
if [[ "$STATE" == *inactive* ]]; then
    ros2 lifecycle set /competition_static_map_server activate >/dev/null
fi

echo "当前地图文件: $MAP"
echo "当前生命周期状态:"
ros2 lifecycle get /competition_static_map_server

echo "正在打开 RViz2..."
rviz2 -d "$ROOT/preview_map.rviz"
