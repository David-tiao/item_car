#!/usr/bin/env bash

ROOT="$HOME/Desktop/ydlidar_ws/maps/competition_v1"

bash "$ROOT/view_static_map.sh"
STATUS=$?

if [[ $STATUS -ne 0 ]]; then
    echo "启动失败，退出码：$STATUS"
    read -r -p "按回车键关闭窗口..." _
fi

exit "$STATUS"
