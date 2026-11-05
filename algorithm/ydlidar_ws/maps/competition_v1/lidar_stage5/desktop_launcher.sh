#!/usr/bin/env bash

S5="$HOME/Desktop/ydlidar_ws/maps/competition_v1/lidar_stage5"

bash "$S5/start_lidar_stage5.sh"
status=$?

if [[ "$status" -ne 0 ]]; then
    read -r -p "启动失败，按回车关闭终端..." _
fi

exit "$status"
