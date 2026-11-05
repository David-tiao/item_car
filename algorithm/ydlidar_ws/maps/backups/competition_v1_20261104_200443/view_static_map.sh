#!/usr/bin/env bash
# 单独预览静态地图；不要与 GMapping 同时运行（都要发布 /map）。
set -Eeo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAP="$ROOT/competition_navigation_map.yaml"

if [[ ! -r /opt/ros/humble/setup.bash ]]; then
  echo '缺少 ROS 2 Humble 环境' >&2
  exit 1
fi
source /opt/ros/humble/setup.bash
if ! ros2 pkg prefix nav2_map_server >/dev/null 2>&1; then
  echo '请先安装：sudo apt install ros-humble-nav2-map-server' >&2
  exit 1
fi
if [[ ! -f "$MAP" ]]; then
  echo "地图文件不存在：$MAP" >&2
  exit 1
fi

map_pid=''
cleanup() {
  trap - EXIT INT TERM
  if [[ -n "$map_pid" ]]; then
    kill "$map_pid" 2>/dev/null || true
    wait "$map_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

echo '注意：请先停止正在发布 /map 的 GMapping。'
ros2 run nav2_map_server map_server --ros-args \
  -p "yaml_filename:=$MAP" -p topic_name:=map -p frame_id:=map &
map_pid=$!

ready=0
for ((i=0; i<40; i++)); do
  if ros2 lifecycle get /map_server >/dev/null 2>&1; then
    ready=1
    break
  fi
  if ! kill -0 "$map_pid" 2>/dev/null; then
    echo 'map_server 启动失败' >&2
    exit 1
  fi
  sleep 0.25
done
if [[ "$ready" != 1 ]]; then
  echo '等待 map_server 生命周期服务超时' >&2
  exit 1
fi
ros2 lifecycle set /map_server configure
ros2 lifecycle set /map_server activate

echo '地图已加载。RViz2 将设置 Fixed Frame=map、Map Topic=/map。'
echo '仅用于预览已知通行区域，并不意味着已经完成定位或导航。'
rviz2 -d "$ROOT/preview_map.rviz"
