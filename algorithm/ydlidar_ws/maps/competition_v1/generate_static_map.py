#!/usr/bin/env python3
from pathlib import Path
import json

OUT = Path(__file__).resolve().parent

# =========================
# 基本参数
# =========================
RES = 0.01          # 1 cm/pixel
SIZE = 280          # 2.8m x 2.8m image
ORIGIN = -0.2       # map origin
FIELD = 2.4         # field size 2.4m x 2.4m

# =========================
# 场地规则几何（第一版最终整理）
# =========================

# 四个黄色禁行方块（田字中间四块）
YELLOW = [
    (0.55, 0.55, 1.00, 1.00),   # 左下
    (1.40, 0.55, 1.85, 1.00),   # 右下
    (0.55, 1.40, 1.00, 1.85),   # 左上
    (1.40, 1.40, 1.85, 1.85),   # 右上
]

# 蓝色启停区（允许）
BLUE = [
    (2.10, 2.10, 2.40, 2.40),   # 右上启停区
    (2.10, 0.00, 2.40, 0.30),   # 右下启停区
]

# 暂存区、粗加工区（固定功能区）
WORK = [
    (0.00, 0.91, 0.15, 1.49),   # 左侧暂存区
    (0.91, 0.00, 1.49, 0.15),   # 下侧粗加工区
]

# 原料圆盘（固定，已修正位置，不压边）
RAW_DISK = {
    "center": (1.20, 2.40),
    "radius": 0.15
}

# 二维码板位置不固定，只在彩色图里做提示
QR_NOTE = {
    "text": "二维码板位置待定",
    "x": 1.98,
    "y": 1.20
}

# 色环示意位置（彩色预览专用）
STORAGE_RINGS = [
    (0.08, 1.34, "red"),
    (0.08, 1.20, "green"),
    (0.08, 1.06, "blue"),
]

PROCESS_RINGS = [
    (1.05, 0.08, "red"),
    (1.20, 0.08, "green"),
    (1.35, 0.08, "blue"),
]

# =========================
# 工具函数
# =========================
def inside_rect(x, y, rect):
    x0, y0, x1, y1 = rect
    return x0 <= x < x1 and y0 <= y < y1

def inside_circle(x, y, center, radius):
    cx, cy = center
    return (x - cx) ** 2 + (y - cy) ** 2 <= radius ** 2

def in_field(x, y):
    return 0 <= x < FIELD and 0 <= y < FIELD

def blocked_physical(x, y):
    """
    物理基础图：
    - 场地外：unknown
    - 场地内：free
    - 固定原料圆盘：occupied
    不把黄色区域当物理墙。
    """
    if inside_circle(x, y, RAW_DISK["center"], RAW_DISK["radius"]):
        return "occupied"

    if not in_field(x, y):
        return "unknown"

    if inside_circle(x, y, RAW_DISK["center"], RAW_DISK["radius"]):
        return "occupied"

    return "free"

def blocked_rules(x, y):
    """
    规则预览/禁行掩膜：
    - 场地外：unknown（用于显示）
    - 黄色区域：occupied
    - 暂存区/粗加工区：occupied
    - 固定圆盘：occupied
    - 蓝色启停区：free
    - 其余灰色通道：free
    """
    if inside_circle(x, y, RAW_DISK["center"], RAW_DISK["radius"]):
        return "occupied"

    if not in_field(x, y):
        return "unknown"

    if any(inside_rect(x, y, r) for r in BLUE):
        return "free"

    if any(inside_rect(x, y, r) for r in YELLOW):
        return "occupied"

    if any(inside_rect(x, y, r) for r in WORK):
        return "occupied"

    if inside_circle(x, y, RAW_DISK["center"], RAW_DISK["radius"]):
        return "occupied"

    return "free"

def cell_value(state):
    # map_server / RViz 占据图常用：
    # occupied = 0
    # free = 254
    # unknown = 205
    if state == "occupied":
        return 0
    if state == "free":
        return 254
    return 205

def generate_map(name, mode):
    data = bytearray()

    for row in range(SIZE):
        y = ORIGIN + (SIZE - row - 0.5) * RES
        for col in range(SIZE):
            x = ORIGIN + (col + 0.5) * RES

            if mode == "physical":
                state = blocked_physical(x, y)
            else:
                state = blocked_rules(x, y)

            data.append(cell_value(state))

    with (OUT / f"{name}.pgm").open("wb") as f:
        f.write(f"P5\n{SIZE} {SIZE}\n255\n".encode())
        f.write(data)

    (OUT / f"{name}.yaml").write_text(
        f"image: {name}.pgm\n"
        "mode: trinary\n"
        f"resolution: {RES}\n"
        f"origin: [{ORIGIN}, {ORIGIN}, 0.0]\n"
        "negate: 0\n"
        "occupied_thresh: 0.65\n"
        "free_thresh: 0.196\n"
    )

def fx(x):
    return 60 + x / FIELD * 680

def fy(y):
    return 740 - y / FIELD * 680

def rect_svg(x0, y0, x1, y1, fill, stroke="#333", sw=2, opacity=1.0):
    sx0 = fx(x0)
    sy1 = fy(y0)
    sx1 = fx(x1)
    sy0 = fy(y1)
    return (
        f'<rect x="{sx0:.1f}" y="{sy0:.1f}" '
        f'width="{sx1-sx0:.1f}" height="{sy1-sy0:.1f}" '
        f'fill="{fill}" stroke="{stroke}" stroke-width="{sw}" opacity="{opacity}"/>\n'
    )

def circle_svg(cx, cy, r, fill, stroke="#333", sw=2, opacity=1.0):
    return (
        f'<circle cx="{fx(cx):.1f}" cy="{fy(cy):.1f}" '
        f'r="{r/FIELD*680:.1f}" fill="{fill}" stroke="{stroke}" '
        f'stroke-width="{sw}" opacity="{opacity}"/>\n'
    )

def text_svg(x, y, text, size=18, color="#111", rotate=None):
    if rotate is None:
        return (
            f'<text x="{fx(x):.1f}" y="{fy(y):.1f}" '
            f'font-size="{size}" fill="{color}" text-anchor="middle">{text}</text>\n'
        )
    return (
        f'<text x="{fx(x):.1f}" y="{fy(y):.1f}" '
        f'font-size="{size}" fill="{color}" text-anchor="middle" '
        f'transform="rotate({rotate},{fx(x):.1f},{fy(y):.1f})">{text}</text>\n'
    )

def generate_color_preview():
    svg = []
    svg.append('<?xml version="1.0" encoding="UTF-8"?>\n')
    svg.append('<svg xmlns="http://www.w3.org/2000/svg" width="800" height="800" viewBox="0 0 800 800">\n')
    svg.append('<rect x="0" y="0" width="800" height="800" fill="#f8f8f8"/>\n')

    # 场地灰色底
    svg.append(rect_svg(0, 0, FIELD, FIELD, fill="#d8d8d8", stroke="#222", sw=3))

    # 黄色区域
    for r in YELLOW:
        svg.append(rect_svg(*r, fill="#f1e66b", stroke="#666", sw=1.5))
    svg.append(text_svg(1.20, 1.22, "田字形禁行区", size=22, color="#555"))

    # 蓝色启停区
    for i, r in enumerate(BLUE, start=1):
        svg.append(rect_svg(*r, fill="#1857ff", stroke="#0d2fa0", sw=2))
        cx = (r[0] + r[2]) / 2
        cy = (r[1] + r[3]) / 2
        svg.append(text_svg(cx, cy, f"启停区{i}", size=18, color="white"))

    # 暂存区
    svg.append(rect_svg(*WORK[0], fill="#ffffff", stroke="#444", sw=2))
    svg.append(text_svg(0.075, 1.20, "暂存区", size=18, color="#111", rotate=-90))

    # 粗加工区
    svg.append(rect_svg(*WORK[1], fill="#ffffff", stroke="#444", sw=2))
    svg.append(text_svg(1.20, 0.08, "粗加工区", size=18, color="#111"))

    # 原料圆盘（修正后位置）
    svg.append(circle_svg(RAW_DISK["center"][0], RAW_DISK["center"][1], RAW_DISK["radius"], fill="#fff8dc", stroke="#111", sw=2))
    svg.append(text_svg(0.65, 2.27, "原料区圆盘", size=18, color="#111"))

    # 色环
    for x, y, c in STORAGE_RINGS:
        svg.append(circle_svg(x, y, 0.03, fill="none", stroke=c, sw=5))
    for x, y, c in PROCESS_RINGS:
        svg.append(circle_svg(x, y, 0.03, fill="none", stroke=c, sw=5))

    # 二维码说明
    svg.append(text_svg(QR_NOTE["x"], QR_NOTE["y"], "二维码板位置待定", size=16, color="#333", rotate=-90))

    # 标题
    svg.append('<text x="400" y="785" font-size="24" fill="#111" text-anchor="middle">工创智能搬运比赛场地静态地图 V5（彩色预览）</text>\n')
    svg.append('</svg>\n')

    (OUT / "field_preview_color.svg").write_text("".join(svg), encoding="utf-8")

def generate_readme():
    txt = """# 工创比赛静态地图 V5

## 文件说明

- competition_navigation_map.yaml
  物理基础图：
  - 场地外为 unknown
  - 场地内为 free
  - 固定原料圆盘为 occupied
  - 黄色区域不作为物理墙

- competition_keepout_mask.yaml
  禁行掩膜：
  - 黄色区域
  - 暂存区
  - 粗加工区
  - 固定原料圆盘
  - 场地外 unknown

- competition_rules_preview.yaml
  规则预览图：
  用于在 RViz 中查看整体占据布局

- field_preview_color.svg
  彩色预览图：
  - 黄色田字块
  - 蓝色启停区
  - 原料区圆盘
  - 暂存区、粗加工区
  - 色环
  - 二维码板位置待定说明

## 说明

1. RViz 中的 /map 是 OccupancyGrid，因此只能黑白/灰度显示。
2. 彩色布局请直接查看 field_preview_color.svg。
3. 原料圆盘位置已修正，不再压在边界线上。
4. 二维码位置不固定，因此不固化进障碍图。
"""
    (OUT / "README.md").write_text(txt, encoding="utf-8")

def generate_json():
    data = {
        "resolution": RES,
        "origin": [ORIGIN, ORIGIN, 0.0],
        "field_size": [FIELD, FIELD],
        "yellow": YELLOW,
        "blue": BLUE,
        "work": WORK,
        "raw_disk": RAW_DISK,
        "qr_fixed": False,
        "storage_rings": STORAGE_RINGS,
        "process_rings": PROCESS_RINGS,
        "note": "最终版 V5，规则图整理版"
    }
    (OUT / "zones.json").write_text(
        json.dumps(data, ensure_ascii=False, indent=2),
        encoding="utf-8"
    )

generate_map("competition_navigation_map", "physical")
generate_map("competition_keepout_mask", "rules")
generate_map("competition_rules_preview", "rules")
generate_color_preview()
generate_readme()
generate_json()

print("已生成最终版静态地图 V5：", OUT)
