# 工创比赛静态地图 V5

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
