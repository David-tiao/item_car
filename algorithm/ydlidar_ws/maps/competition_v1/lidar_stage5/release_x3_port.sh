#!/usr/bin/env bash
set -Eeo pipefail

PORT="${1:-/dev/ttyUSB0}"
MODE="${2:-release}"

PORT="$(readlink -f -- "$PORT")"

[[ -c "$PORT" ]] || {
    echo "[串口] 设备不存在：$PORT"
    echo "请检查 CP2102 是否连接"
    exit 1
}

owners() {
    local value rc

    value="$(fuser -n file "$PORT" 2>/dev/null)" &&
        rc=0 || rc=$?

    [[ "$rc" == 0 || "$rc" == 1 ]] || {
        echo "[串口] fuser 检查失败：$rc" >&2
        return 2
    }

    [[ "$value" =~ ^[[:space:][:digit:]]*$ ]] || {
        echo "[串口] 无法解析占用 PID" >&2
        return 2
    }

    printf '%s\n' "$value"
}

is_x3() {
    local pid="$1"
    local exe uid

    [[ -r "/proc/$pid/stat" ]] || return 1

    uid="$(stat -c %u "/proc/$pid" 2>/dev/null || true)"

    [[ "$uid" == "$EUID" ]] || return 1

    exe="$(readlink -f "/proc/$pid/exe" 2>/dev/null || true)"

    [[ "${exe##*/}" == "ydlidar_ros2_driver_node" ]]
}

proc_start() {
    local pid="$1"

    [[ -r "/proc/$pid/stat" ]] || return 1

    sed 's/^.*) //' "/proc/$pid/stat" | awk '{print $20}'
}

raw="$(owners)" || exit 1
read -r -a pids <<< "$raw"

if (( ${#pids[@]} == 0 )); then
    echo "[串口] $PORT 当前空闲"
    exit 0
fi

echo "[串口] 检测到占用：${pids[*]}"

if [[ "$MODE" == "--check-only" ]]; then
    echo "[串口] 仍被占用，拒绝启动第二个驱动"
    fuser -v "$PORT" || true
    exit 1
fi

# 确认所有占用者都是本用户的旧 X3 驱动
declare -A birth=()

for pid in "${pids[@]}"; do

    if ! is_x3 "$pid"; then
        echo "[串口] PID=$pid 不是确认的 X3 驱动"
        echo "[串口] 为避免误杀其他程序，停止自动清理"
        fuser -v "$PORT" || true
        exit 1
    fi

    birth["$pid"]="$(proc_start "$pid")" || exit 1
done

# 第一次：正常终止
for pid in "${pids[@]}"; do
    if [[ "$(proc_start "$pid" 2>/dev/null || true)" == "${birth[$pid]}" ]]; then
        echo "[串口] 执行 kill -TERM $pid"
        kill -TERM "$pid" 2>/dev/null || true
    fi
done

# 等待串口释放
for i in {1..40}; do

    raw="$(owners)" || exit 1
    read -r -a held <<< "$raw"

    if (( ${#held[@]} == 0 )); then
        echo "[串口] 串口已释放：$PORT"
        exit 0
    fi

    sleep 0.1
done

# 第二次：仍未退出的已确认旧驱动才执行 KILL
raw="$(owners)" || exit 1
read -r -a held <<< "$raw"

for pid in "${held[@]}"; do

    if [[ -n "${birth[$pid]:-}" ]] &&
       is_x3 "$pid" &&
       [[ "$(proc_start "$pid" 2>/dev/null || true)" == "${birth[$pid]}" ]]; then

        echo "[串口] 执行 kill -KILL $pid"
        kill -KILL "$pid" 2>/dev/null || true

    else
        echo "[串口] PID=$pid 不是原来确认的驱动，不执行 KILL"
    fi

done

sleep 0.4

raw="$(owners)" || exit 1
read -r -a held <<< "$raw"

if (( ${#held[@]} > 0 )); then
    echo "[串口] 串口仍然被占用：${held[*]}"
    fuser -v "$PORT" || true
    exit 1
fi

echo "[串口] 已成功释放 $PORT"
