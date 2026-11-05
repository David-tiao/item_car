#!/usr/bin/env python3
from pathlib import Path
import json
import math

OUT = Path(__file__).resolve().parent

# =========================
# 基本参数
# =========================
RES = 0.01          # 1 cm / pixel
SIZE = 280          # 2.8 m map image
ORIGIN = -0.2       # map origin
FIELD = 2.4         # 2.4 x 2.4 m

# =========================
# 规则区域（基于规则图的第一版几何）
# =========================

# 四个黄色禁行方块
YELLOW = [
    (0.55, 0.55, 1.00, 1.00),   # 左下
    (1.40, 0.55, 1.85, 1.00),   # 右下
    (0.55, 1.40, 1.00, 1.85),   # 左上
    (1.40, 1.40, 1.85, 1.85),   # 右上
]

# 启停区（蓝色，允许停放/出发）
BLUE = [
    (2.10, 2.10, 2.40, 2.40),   # 启停区1
    (2.10, 0.00, 2.40, 0.30),   # 启停区2
]

# 暂存区、粗加工区（固定）
WORK = [
    (0.00, 0.91, 0.15, 1.49),   # 暂存区
    (0.91, 0.00, 1.49, 0.15),   # 粗加工区
]

# 原料区圆盘（固定）
RAW_DISK = {
    "center": (1.20, 2.25),
    "radius": 0.15
}

# 二维码板：位置不固定，这里不写死为障碍，仅在彩色预览中说明
QR_NOTE = {
    "text": "二维码板位置待定",
    "x": 1.95,
    "y": 1.20
}

# 色环位置（示意，彩色预览使用）
STORAGE_RINGS = [
    (0.08, 1.32, "red"),
    (0.08, 1.18, "green"),
    (0.08, 1.04, "blue"),
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

def blocked_rules(x, y):
    """
    比赛规则禁行掩膜:
    - 场地外禁止
    - 黄色区域禁止
    - 暂存区、粗加工区禁止
    - 原料圆盘位置固定，作为固定实体区域，禁止驶入
    - 启停区允许
    """
    if not (0 <= x < FIELD and 0 <= y < FIELD):
        return True

    if any(inside_rect(x, y, r) for r in BLUE):
        return False

    if any(inside_rect(x, y, r) for r in YELLOW):
        return True

    if any(inside_rect(x, y, r) for r in WORK):
        return True

    if inside_circle(x, y, RAW_DISK["center"], RAW_DISK["radius"]):
        return True

    return False

def blocked_physical(x, y):
    """
    物理基础图：
    - 只保留场地边界
    - 不把黄色地面颜色当成实体墙
    - 只把固定实体圆盘作为物理占据
    """
    if not (0 <= x < FIELD and 0 <= y < FIELD):
        return True

    if inside_circle(x, y, RAW_DISK["center"], RAW_DISK["radius"]):
        return True

    return False

def save_pgm_yaml(name, mode="rules"):
    data = bytearray()

    for row in range(SIZE):
        y = ORIGIN + (SIZE - row - 0.5) * RES
        for col in range(SIZE):
            x = ORIGIN + (col + 0.5) * RES

            if mode == "physical":
                occ = blocked_physical(x, y)
            else:
                occ = blocked_rules(x, y)

            data.append(0 if occ else 254)

    pgm_path = OUT / f"{name}.pgm"
    yaml_path = OUT / f"{name}.yaml"

    with pgm_path.open("wb") as f:
        f.write(f"P5\n{SIZE} {SIZE}\n255\n".encode())
        f.write(data)

    yaml_path.write_text(
        f"image: {name}.pgm\n"
        "mode: trinary\n"
        f"resolution: {RES}\n"
        f"origin: [{ORIGIN}, {ORIGIN}, 0.0]\n"
        "negate: 0\n"
        "occupied_thresh: 0.65\n"
        "free_thresh: 0.196\n"
    )

# =========================
# 生成彩色 SVG 预览
# =========================
def field_to_svg_x(x):
    return 60 + x / FIELD * 680

def field_to_svg_y(y):
    return 740 - y / FIELD * 680

def rect_svg(x0, y0, x1, y1, fill, stroke="#333", sw=2, opacity=1.0):
    sx0 = field_to_svg_x(x0)
    sy1 = field_to_svg_y(y0)
    sx1 = field_to_svg_x(x1)
    sy0 = field_to_svg_y(y1)
    w = sx1 - sx0
    h = sy1 - sy0
    return f'<rect x="{sx0:.1f}" y="{sy0:.1f}" width="{w:.1f}" height="{h:.1f}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" opacity="{opacity}"/>\n'

def circle_svg(cx, cy, r, fill, stroke="#333", sw=2, opacity=1.0):
    sx = field_to_svg_x(cx)
    sy = field_to_svg_y(cy)
    sr = r / FIELD * 680
    return f'<circle cx="{sx:.1f}" cy="{sy:.1f}" r="{sr:.1f}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" opacity="{opacity}"/>\n'

def text_svg(x, y, text, size=18, color="#111", rotate=None):
    sx = field_to_svg_x(x)
    sy = field_to_svg_y(y)
    if rotate is None:
        return f'<text x="{sx:.1f}" y="{sy:.1f}" font-size="{size}" fill="{color}" text-anchor="middle">{text}</text>\n'
    else:
        return f'<text x="{sx:.1f}" y="{sy:.1f}" font-size="{size}" fill="{color}" text-anchor="middle" transform="rotate({rotate},{sx:.1f},{sy:.1f})">{text}</text>\n'

def generate_svg():
    parts = []
    parts.append('<?xml version="1.0" encoding="UTF-8"?>\n')
    parts.append('<svg xmlns="http://www.w3.org/2000/svg" width="800" height="800" viewBox="0 0 800 800">\n')
    parts.append('<rect x="0" y="0" width="800" height="800" fill="#f7f7f7"/>\n')

    # 场地底色（灰色）
    parts.append(rect_svg(0, 0, FIELD, FIELD, fill="#d9d9d9", stroke="#222", sw=3))

    # 黄色方块
    for r in YELLOW:
        parts.append(rect_svg(*r, fill="#f3ea8a", stroke="#666", sw=1.5))
    parts.append(text_svg(1.20, 1.22, "田字形禁行区", size=22, color="#444"))

    # 蓝色启停区
    for i, r in enumerate(BLUE, start=1):
        parts.append(rect_svg(*r, fill="#0b46ff", stroke="#0833aa", sw=2))
        cx = (r[0] + r[2]) / 2
        cy = (r[1] + r[3]) / 2
        parts.append(text_svg(cx, cy, f"启停区{i}", size=20, color="white"))

    # 暂存区、粗加工区
    parts.append(rect_svg(*WORK[0], fill="#ffffff", stroke="#444", sw=2))
    parts.append(text_svg(0.08, 1.20, "暂存区", size=18, color="#111", rotate=-90))

    parts.append(rect_svg(*WORK[1], fill="#ffffff", stroke="#444", sw=2))
    parts.append(text_svg(1.20, 0.09, "粗加工区", size=18, color="#111"))

    # 原料圆盘
    parts.append(circle_svg(RAW_DISK["center"][0], RAW_DISK["center"][1], RAW_DISK["radius"], fill="#fffdf2", stroke="#111", sw=2))
    parts.append(text_svg(1.20, 2.33, "原料区圆盘", size=18, color="#111"))

    # 色环
    for x, y, c in STORAGE_RINGS:
        parts.append(circle_svg(x, y, 0.035, fill="none", stroke=c, sw=5))
    for x, y, c in PROCESS_RINGS:
        parts.append(circle_svg(x, y, 0.035, fill="none", stroke=c, sw=5))

    # 二维码板：不固定，只提示
    parts.append(text_svg(2.02, 1.20, "二维码板\n位置待定", size=16, color="#333", rotate=-90))

    # 边框标注
    parts.append(text_svg(1.20, -0.10, "工创智能搬运比赛场地静态地图 V4（彩色预览）", size=22, color="#111"))
    parts.append('</svg>\n')

    (OUT / "field_preview_color.svg").write_text("".join(parts), encoding="utf-8")

# =========================
# 生成 README
# =========================
def generate_readme():
    text = """# 工创比赛静态地图 V4

## 文件说明

- competition_navigation_map.yaml  
  物理基础地图。  
  只保留场地边界和固定圆盘，不把黄色区域当地形墙。

- competition_keepout_mask.yaml  
  比赛规则禁行掩膜。  
  包含黄色禁行区、暂存区、粗加工区、固定圆盘、场地外。

- competition_rules_preview.yaml  
  规则预览图对应的 OccupancyGrid（黑白）。

- field_preview_color.svg  
  彩色规则预览图。  
  包含：
  - 黄色田字区
  - 蓝色启停区
  - 原料区圆盘
  - 暂存区与粗加工区
  - 色环位置
  - 二维码板位置待定说明

## 注意

1. RViz 里的 /map 是 OccupancyGrid，本身只能黑白/灰度显示，不能直接像图片一样彩色。
2. 如果要看彩色布局，请直接打开 field_preview_color.svg。
3. 原料圆盘位置按固定位置写入。
4. 二维码板位置不固定，因此不写死为障碍。
"""
    (OUT / "README.md").write_text(text, encoding="utf-8")

# =========================
# 输出 JSON
# =========================
def generate_json():
    data = {
        "resolution": RES,
        "origin": [ORIGIN, ORIGIN, 0.0],
        "field_size": [FIELD, FIELD],
        "yellow": YELLOW,
        "blue": BLUE,
        "work": WORK,
        "raw_disk": RAW_DISK,
        "qr_board_fixed": False,
        "storage_rings": STORAGE_RINGS,
        "process_rings": PROCESS_RINGS,
        "note": "几何位置基于规则图第一版整理，后续可再微调"
    }
    (OUT / "zones.json").write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")

# =========================
# 主程序
# =========================
save_pgm_yaml("competition_navigation_map", mode="physical")
save_pgm_yaml("competition_keepout_mask", mode="rules")
save_pgm_yaml("competition_rules_preview", mode="rules")
generate_svg()
generate_readme()
generate_json()

print("已生成新版静态地图 V4：", OUT)
