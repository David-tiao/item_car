#!/usr/bin/env bash
set -Eeo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source /opt/ros/humble/setup.bash

exec 9>"$ROOT/.blue_zones.lock"
flock -n 9 || {
    echo "静态地图已经运行，请先关闭旧窗口"
    exit 1
}

mkdir -p "$ROOT/logs"

python3 "$ROOT/publish_blue_start_zones.py" \
  > "$ROOT/logs/blue_zones.log" 2>&1 &
BLUE_PID=$!

cleanup() {
    trap - EXIT INT TERM HUP
    kill "$BLUE_PID" 2>/dev/null || true
    wait "$BLUE_PID" 2>/dev/null || true
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

bash "$ROOT/view_static_map_core.sh" "$@"
