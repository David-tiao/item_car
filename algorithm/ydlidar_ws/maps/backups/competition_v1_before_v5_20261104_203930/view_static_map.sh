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

pkill -f 'competition_static_map_server' 2>/dev/null || true
sleep 1

ros2 run nav2_map_server map_server --ros-args \
  -r __node:=competition_static_map_server \
  -p "yaml_filename:=$MAP" \
  -p topic_name:=map \
  -p frame_id:=map &
MAP_PID=$!

cleanup() {
    kill $MAP_PID 2>/dev/null || true
}
trap cleanup EXIT INT TERM

for i in $(seq 1 20); do
    if ros2 lifecycle get /competition_static_map_server >/dev/null 2>&1; then
        break
    fi
    sleep 0.4
done

STATE=$(ros2 lifecycle get /competition_static_map_server 2>/dev/null || true)
if [[ "$STATE" == *unconfigured* ]]; then
    ros2 lifecycle set /competition_static_map_server configure
fi

STATE=$(ros2 lifecycle get /competition_static_map_server 2>/dev/null || true)
if [[ "$STATE" == *inactive* ]]; then
    ros2 lifecycle set /competition_static_map_server activate
fi

echo "当前地图:"
echo "  $MAP"
echo "生命周期状态:"
ros2 lifecycle get /competition_static_map_server

rviz2
