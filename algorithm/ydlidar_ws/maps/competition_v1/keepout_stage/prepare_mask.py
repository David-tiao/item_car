#!/usr/bin/env python3
"""从已核对过的 V5 禁行掩膜派生 Nav2 安全掩膜，不碰原图。
   源：0=禁止、254=允许、205=未知；目标：未知一律禁止（含场外）。
"""
from pathlib import Path
import re

stage = Path(__file__).resolve().parent
root = stage.parent
src = root / 'competition_keepout_mask.pgm'
yaml_src = root / 'competition_keepout_mask.yaml'
if not src.exists() or not yaml_src.exists():
    raise SystemExit('缺少原禁行掩膜的 PGM 或 YAML')
with src.open('rb') as f:
    header = f.readline().strip()
    if header != b'P5':
        raise SystemExit(f'只支持二进制 P5 PGM，实际为 {header!r}')
    size_line = f.readline().strip()
    while size_line.startswith(b'#'):
        size_line = f.readline().strip()
    w, h = [int(x) for x in size_line.split()]
    mx = int(f.readline().strip())
    data = f.read()
if mx != 255 or len(data) != w*h:
    raise SystemExit('PGM 头信息或像素长度错误')
if (w, h) != (280, 280):
    raise SystemExit(f'当前地图大小是 {w}x{h}，预期 V5 为 280x280。停止以免误改。')
# 严格禁行：只有纯白（自由）保持允许；黑色/未知/非纯白一律标为障碍
converted = bytes(254 if p == 254 else 0 for p in data)
name = 'competition_keepout_nav2'
(stage / (name + '.pgm')).write_bytes(b'P5\n280 280\n255\n' + converted)
yaml = yaml_src.read_text(encoding='utf-8')
yaml, n = re.subn(r'(?m)^image\s*:\s*.*$', 'image: ' + name + '.pgm', yaml, count=1)
if n != 1:
    raise SystemExit('原 YAML 缺少 image 字段')
(stage / (name + '.yaml')).write_text(yaml, encoding='utf-8')

# 按现有场地坐标进行静态像素验收
# 图像行序：上->下；地图坐标原点：[-0.2, -0.2]
def sample(x,y):
    col = int((x + 0.2) / 0.01)
    row = h - 1 - int((y + 0.2) / 0.01)
    return converted[row*w+col]
checks = [
    ('中心车道', 1.20, 1.20, 254),
    ('左下黄色区', 0.75, 0.75, 0),
    ('右上黄色区', 1.60, 1.60, 0),
    ('蓝色启停区1', 2.25, 2.25, 254),
    ('蓝色启停区2', 2.25, 0.15, 254),
    ('左侧作业区', 0.08, 1.20, 0),
    ('场地外部', -0.10, 1.20, 0),
]
for desc,x,y,expected in checks:
    val = sample(x,y)
    print(f'{desc:<12} = {val:3d}  ' + ('通过' if val == expected else '错误'))
    if val != expected:
        raise SystemExit('禁行掩膜检测失败：' + desc)
print('Nav2 专用禁行掩膜生成成功，原来的彩色预览及 V5 地图均未改动。')
