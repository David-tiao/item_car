#!/usr/bin/env python3
"""2027 工创赛智能搬运初赛：规则示意图的静态通行地图 V1。

标准库即可运行。没有将颜色标线伪装成可由雷达测得的物理墙体。
运行: python3 generate_static_map.py [--out 输出目录]
"""
from pathlib import Path
import argparse
import json

FIELD_SIZE = 2.40                 # m；规则明确值
RESOLUTION = 0.01                # m/pixel；工程选择
MARGIN = 0.20                    # m，地图外围显式禁行带
ORIGIN = (-MARGIN, -MARGIN)
PIXELS = int(round((FIELD_SIZE + 2 * MARGIN) / RESOLUTION))

# 黄色方块边长 0.45 m、十字 0.40 m；位置按居中对称示意图推算，非现场测量值。
YELLOW = [
    dict(id="yellow_SW", x0=0.55, y0=0.55, x1=1.00, y1=1.00),
    dict(id="yellow_SE", x0=1.40, y0=0.55, x1=1.85, y1=1.00),
    dict(id="yellow_NW", x0=0.55, y0=1.40, x1=1.00, y1=1.85),
    dict(id="yellow_NE", x0=1.40, y0=1.40, x1=1.85, y1=1.85),
]
# 图 1 标注的 580x150 mm 粗加工区/暂存区沿场地边缘，图示居中近似。
WORK = [
    dict(id="storage_white", x0=0.00, y0=0.91, x1=0.15, y1=1.49),
    dict(id="rough_work_white", x0=0.91, y0=0.00, x1=1.49, y1=0.15),
]
# 蓝色启停区 300x300mm：图示右上和右下，作为合法出发/返回区。
START = [
    dict(id="start_NE", x0=2.10, y0=2.10, x1=2.40, y1=2.40),
    dict(id="start_SE", x0=2.10, y0=0.00, x1=2.40, y1=0.30),
]


def within(rect, x, y):
    return rect['x0'] <= x < rect['x1'] and rect['y0'] <= y < rect['y1']


def occupancy(x, y):
    """地图值：100=禁行，0=允许中心点；没有绘制随机障碍物。"""
    if not (0.0 <= x < FIELD_SIZE and 0.0 <= y < FIELD_SIZE):
        return 100
    if any(within(r, x, y) for r in YELLOW + WORK):
        return 100
    return 0


def pgm_and_yaml(folder, base, samples):
    # ROS 地图图像首行是世界 y 最大值，因此沿上到下写入像素行。
    out = folder / (base + '.pgm')
    payload = bytearray(PIXELS * PIXELS)
    idx = 0
    for row in range(PIXELS):
        wy = ORIGIN[1] + (PIXELS - row - 0.5) * RESOLUTION
        for col in range(PIXELS):
            wx = ORIGIN[0] + (col + 0.5) * RESOLUTION
            occ = occupancy(wx, wy)
            payload[idx] = 0 if occ == 100 else 254
            idx += 1
    out.write_bytes(f'P5\n{PIXELS} {PIXELS}\n255\n'.encode('ascii') + payload)
    (folder / (base + '.yaml')).write_text(
        f'image: {base}.pgm\n'
        'mode: trinary\n'
        f'resolution: {RESOLUTION}\n'
        f'origin: [{ORIGIN[0]}, {ORIGIN[1]}, 0.0]\n'
        'negate: 0\noccupied_thresh: 0.65\nfree_thresh: 0.196\n',
        encoding='utf-8')
    samples[base] = {'blocked_pixels': sum(v == 0 for v in payload),
                     'free_pixels': sum(v == 254 for v in payload)}


def svg_preview(folder):
    # 仅供人工检查的彩色平面图，不是 ROS 地图；工作站和 QR 标注为示意。
    left, top, unit = 92, 102, 240
    sx = lambda x: left + x * unit
    sy = lambda y: top + (FIELD_SIZE - y) * unit
    def rect(z, fill, stroke='none'):
        x0, y0, x1, y1 = [z[k] for k in ('x0', 'y0', 'x1', 'y1')]
        return (f'<rect x="{sx(x0):.1f}" y="{sy(y1):.1f}" '
                f'width="{(x1-x0)*unit:.1f}" height="{(y1-y0)*unit:.1f}" '
                f'fill="{fill}" stroke="{stroke}"/>')
    nodes = ['''<svg xmlns="http://www.w3.org/2000/svg" width="980" height="830" viewBox="0 0 980 830">
<rect width="980" height="830" fill="#fafafa"/>
<text x="92" y="48" font-size="25" font-weight="bold" fill="#172334">工创智能搬运 · 初赛静态场地 V1</text>
<text x="92" y="75" font-size="14" fill="#64748b">2400 × 2400 mm；图示推算版，必须与现场尺寸核对</text>''',
        f'<rect x="{left-10}" y="{top-10}" width="{2.4*unit+20}" height="{2.4*unit+20}" fill="#eee"/>',
        f'<rect x="{left}" y="{top}" width="{2.4*unit}" height="{2.4*unit}" fill="#c6c9cc" stroke="#334155" stroke-width="3"/>']
    for z in YELLOW:
        nodes.append(rect(z, '#fff1a8', '#bca560'))
    for z in WORK:
        nodes.append(rect(z, '#ffffff', '#8b8b8b'))
    for z in START:
        nodes.append(rect(z, '#1b50f0', '#0c36a9'))
    for y in [0, .4, .8, 1.2, 1.6, 2.0, 2.4]:
        nodes.append(f'<line x1="{left}" y1="{sy(y):.1f}" x2="{sx(2.4)}" y2="{sy(y):.1f}" stroke="#ffffff" stroke-opacity=".16"/>')
        nodes.append(f'<text x="{left-12}" y="{sy(y)+4:.1f}" font-size="13" text-anchor="end" fill="#3f4b58">{y:.1f}</text>')
    for x in [0, .4, .8, 1.2, 1.6, 2.0, 2.4]:
        nodes.append(f'<line x1="{sx(x):.1f}" y1="{top}" x2="{sx(x):.1f}" y2="{sy(0)}" stroke="#ffffff" stroke-opacity=".16"/>')
        nodes.append(f'<text x="{sx(x):.1f}" y="{sy(0)+22:.1f}" font-size="13" text-anchor="middle" fill="#3f4b58">{x:.1f}</text>')
    nodes += [
        f'<text x="{sx(1.20)}" y="{sy(1.20)+6}" font-size="17" text-anchor="middle" fill="#334155">十字灰色车道 400 mm</text>',
        f'<text x="{sx(2.25)}" y="{sy(2.25)+6}" font-size="16" fill="white" text-anchor="middle">启停 1</text>',
        f'<text x="{sx(2.25)}" y="{sy(.15)+6}" font-size="16" fill="white" text-anchor="middle">启停 2</text>',
        f'<text x="{sx(.55)}" y="{sy(.78)}" font-size="14" fill="#735e26">禁行</text>',
        f'<text x="{sx(1.40)}" y="{sy(.78)}" font-size="14" fill="#735e26">禁行</text>',
        f'<text x="{sx(.55)}" y="{sy(1.63)}" font-size="14" fill="#735e26">禁行</text>',
        f'<text x="{sx(1.40)}" y="{sy(1.63)}" font-size="14" fill="#735e26">禁行</text>',
        f'<text x="{sx(1.20)}" y="{top-17}" font-size="16" text-anchor="middle" fill="#334155">原料转盘：位置浮动，顶边附近</text>',
        f'<text x="{sx(1.20)}" y="{sy(.22)}" font-size="15" text-anchor="middle" fill="#334155">粗加工区</text>',
        f'<text x="{sx(.24)}" y="{sy(1.16)}" font-size="15" fill="#334155">暂存区</text>',
        f'<text x="{sx(2.4)+13}" y="{sy(1.20)}" font-size="16" fill="#334155">二维码板</text>',
        f'<text x="{sx(2.4)+13}" y="{sy(1.20)+23}" font-size="13" fill="#b45309">位置随机，不是固定导航点</text>',
        '<text x="92" y="776" font-size="14" fill="#475569">灰色：可通行   蓝色：启停区   黄色：禁止驶入   白色站位：作业区域（按图示暂列禁行）</text>',
        '<text x="92" y="799" font-size="13" fill="#b45309">注意：随机黑色圆柱未画入静态地图；二维码任务顺序由扫码获得</text>',
        '</svg>'
    ]
    (folder / 'field_preview.svg').write_text('\n'.join(nodes), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path,
                        default=Path.home() / 'Desktop/ydlidar_ws/maps/competition_v1')
    out = parser.parse_args().out.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)
    summary = {}
    # 同样的几何约束，分成两份独立文件，便于后续分别用于静态地图和 Nav2 Keepout Filter。
    pgm_and_yaml(out, 'competition_navigation_map', summary)
    pgm_and_yaml(out, 'competition_keepout_mask', summary)
    svg_preview(out)
    zones = {
        'unit': 'm', 'frame': 'map', 'origin_at_field_southwest': [0.0, 0.0],
        'field_size': [FIELD_SIZE, FIELD_SIZE], 'resolution': RESOLUTION,
        'map_file_origin': [ORIGIN[0], ORIGIN[1], 0.0],
        'source': 'item.pdf page 3 figure 1 + table 2; field positions inferred from diagram',
        'confirmed_dimensions': {'cross_lane_width': 0.4, 'yellow_side': 0.45,
                                 'start_side': 0.3, 'workstation_length': 0.58,
                                 'workstation_width': 0.15,
                                 'black_cylinder_diameter': 0.05,
                                 'black_cylinder_height': 0.1},
        'yellow_keepout_zones': YELLOW, 'workstation_keepout_zones': WORK,
        'start_zones': START,
        'qr_board': {'position': 'random within specified limits; no fixed map pose',
                     'size_qr_m': [0.08, 0.08]},
        'material_turntable': {'position': 'varies; indicative near north edge',
                               'diameter_m': 0.3},
        'random_black_obstacles': 'Not included in static map; detected online',
        'blocked_cells_meaning': 'non-driving areas, NOT necessarily physical walls for LiDAR matching',
        'critical': 'dimensions/coordinates inferred from drawing must be calibrated on actual field',
        'pixel_counts': summary,
    }
    (out / 'zones.json').write_text(json.dumps(zones, ensure_ascii=False, indent=2), encoding='utf-8')
    print('生成完成：', out)
    for f in sorted(out.iterdir()):
        print('  ', f.name)
    print('禁行像素：', summary['competition_navigation_map']['blocked_pixels'])
    print('注意：不要用这张颜色约束地图直接进行激光 AMCL 定位。')


if __name__ == '__main__':
    main()
