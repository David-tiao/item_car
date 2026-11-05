# 工创比赛静态地图 V4

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
