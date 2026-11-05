#!/usr/bin/env bash
STAGE="$HOME/Desktop/ydlidar_ws/maps/competition_v1/keepout_stage"
bash "$STAGE/start_keepout_stage.sh"
code=$?
if [[ $code -ne 0 ]]; then
  echo "错误码：$code，保留终端供查看。"
  read -r -p '按回车关闭...' _
fi
exit "$code"
