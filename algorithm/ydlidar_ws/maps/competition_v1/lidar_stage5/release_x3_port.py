#!/usr/bin/env python3
import os
import pathlib
import shutil
import signal
import stat
import subprocess
import sys
import time

if len(sys.argv) != 2:
    sys.exit("用法: release_x3_port.py /dev/ttyUSB0")

port = sys.argv[1]

if not pathlib.Path(port).exists():
    sys.exit(f"串口不存在：{port}，请检查 USB / CP2102")

if not stat.S_ISCHR(os.stat(port).st_mode):
    sys.exit(f"不是字符设备：{port}")

if not shutil.which("fuser"):
    sys.exit("缺少 fuser，请执行 sudo apt install psmisc")

def owners():
    p = subprocess.run(
        ["fuser", "-n", "file", port],
        capture_output=True,
        text=True
    )

    if p.returncode not in (0, 1):
        raise RuntimeError(
            f"fuser 检查失败：{p.stderr.strip()}"
        )

    if p.returncode == 1:
        return []

    words = p.stdout.split()

    if not words or any(not x.isdecimal() for x in words):
        raise RuntimeError("无法安全解析占用进程")

    return sorted({int(x) for x in words})

def process_info(pid):
    try:
        base = pathlib.Path(f"/proc/{pid}")

        cmd = (base / "cmdline").read_bytes().split(b"\0")
        cmd = [
            x.decode(errors="replace")
            for x in cmd if x
        ]

        name = os.path.basename(
            os.readlink(base / "exe")
        ).removesuffix(" (deleted)")

        st = (base / "stat").read_text()
        starttime = st.rsplit(") ", 1)[1].split()[19]

        return name, cmd, starttime

    except (OSError, IndexError):
        return None

def is_driver(info):
    if info is None:
        return False

    name, cmd, _ = info

    return (
        name == "ydlidar_ros2_driver_node"
        or (
            bool(cmd)
            and os.path.basename(cmd[0])
            == "ydlidar_ros2_driver_node"
        )
    )

try:
    pids = owners()

    if not pids:
        print(f"[串口] {port} 当前空闲", flush=True)
        sys.exit(0)

    infos = {
        pid: process_info(pid)
        for pid in pids
    }

    print(f"[串口] {port} 被占用：", flush=True)

    for pid, info in infos.items():
        name = info[0] if info else "未知进程"
        print(f"  PID={pid}  COMMAND={name}", flush=True)

    unknown = [
        pid for pid, info in infos.items()
        if not is_driver(info)
    ]

    if unknown:
        sys.exit(
            "[串口] 存在非 X3 雷达驱动进程，"
            "为避免误杀其他设备程序，拒绝自动结束。\n"
            f"请检查：sudo fuser -v {port}"
        )

    # 先温和结束旧驱动
    for pid, info in infos.items():
        current = process_info(pid)

        if (
            current is None
            or current[2] != info[2]
            or not is_driver(current)
        ):
            sys.exit(
                f"[串口] PID {pid} 已变化，停止操作"
            )

        print(
            f"[串口] 自动停止旧 X3 驱动 PID={pid}",
            flush=True
        )

        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        except PermissionError:
            sys.exit(f"没有权限结束 PID={pid}")

    for _ in range(40):
        if not owners():
            print(f"[串口] {port} 已释放", flush=True)
            sys.exit(0)

        time.sleep(0.1)

    # 旧驱动仍不退出时，验证身份后再强制结束
    remaining = set(owners())

    for pid, info in infos.items():
        if pid not in remaining:
            continue

        current = process_info(pid)

        if (
            current is None
            or current[2] != info[2]
            or not is_driver(current)
        ):
            sys.exit(
                f"[串口] PID {pid} 身份发生变化，拒绝强制结束"
            )

        print(
            f"[串口] 旧驱动 PID={pid} 未退出，强制结束",
            flush=True
        )

        try:
            os.kill(pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        except PermissionError:
            sys.exit(f"没有权限强制结束 PID={pid}")

    for _ in range(30):
        if not owners():
            print(
                f"[串口] {port} 释放成功，可以启动雷达",
                flush=True
            )
            sys.exit(0)

        time.sleep(0.1)

    sys.exit(
        f"[串口] 仍被占用，请检查 sudo fuser -v {port}"
    )

except RuntimeError as e:
    sys.exit(f"[串口] {e}")
