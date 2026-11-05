#!/usr/bin/env bash
set -Eeo pipefail

source /opt/ros/humble/setup.bash

S6="$HOME/Desktop/ydlidar_ws/maps/competition_v1/topology_stage6"

exec python3 "$S6/road_topology.py" "$@"
