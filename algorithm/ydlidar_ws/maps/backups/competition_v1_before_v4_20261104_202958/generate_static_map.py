#!/usr/bin/env python3

from pathlib import Path
import json

OUT = Path(__file__).resolve().parent

RES = 0.01
SIZE = 280
ORIGIN = -0.2
FIELD = 2.4

# 根据比赛规则图估算，非现场实测坐标
YELLOW = [
    (0.55, 0.55, 1.00, 1.00),
    (1.40, 0.55, 1.85, 1.00),
    (0.55, 1.40, 1.00, 1.85),
    (1.40, 1.40, 1.85, 1.85)
]

WORK = [
    (0.00, 0.91, 0.15, 1.49),
    (0.91, 0.00, 1.49, 0.15)
]

BLUE = [
    (2.10, 2.10, 2.40, 2.40),
    (2.10, 0.00, 2.40, 0.30)
]

def inside(x, y, rect):
    x0, y0, x1, y1 = rect
    return x0 <= x < x1 and y0 <= y < y1

def blocked(x, y):
    if not (0 <= x < FIELD and 0 <= y < FIELD):
        return True

    if any(inside(x, y, r) for r in BLUE):
        return False

    return any(inside(x, y, r) for r in YELLOW + WORK)

def generate(name, kind):
    data = bytearray()

    for row in range(SIZE):
        y = ORIGIN + (SIZE - row - 0.5) * RES

        for col in range(SIZE):
            x = ORIGIN + (col + 0.5) * RES

            if kind == "physical":
                value = (
                    254 if 0 <= x < FIELD and 0 <= y < FIELD
                    else 205
                )
            else:
                value = 0 if blocked(x, y) else 254

            data.append(value)

    (OUT / f"{name}.pgm").write_bytes(
        f"P5\n{SIZE} {SIZE}\n255\n".encode() + data
    )

    (OUT / f"{name}.yaml").write_text(
        f"image: {name}.pgm\n"
        "mode: trinary\n"
        "resolution: 0.01\n"
        "origin: [-0.2, -0.2, 0.0]\n"
        "negate: 0\n"
        "occupied_thresh: 0.65\n"
        "free_thresh: 0.196\n"
    )

    print(f"已生成：{name}")

generate("competition_navigation_map", "physical")
generate("competition_keepout_mask", "rules")
generate("competition_rules_preview", "rules")

(OUT / "zones.json").write_text(
    json.dumps({
        "field_size_m": [2.4, 2.4],
        "resolution": 0.01,
        "yellow": YELLOW,
        "work": WORK,
        "blue": BLUE,
        "note": "区域坐标来自示意图估算，需现场核实"
    }, ensure_ascii=False, indent=2)
)
