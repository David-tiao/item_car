#!/usr/bin/env bash
S4="$HOME/Desktop/ydlidar_ws/maps/competition_v1/costmap_stage4"
bash "$S4/start_costmap_stage4.sh"
status=$?
if [[ "$status" -ne 0 ]]; then
  echo "第四阶段启动失败，退出码：$status"
  read -r -p '按回车关闭，或复制错误日志后关闭...' _
fi
exit "$status"
