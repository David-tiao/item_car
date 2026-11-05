# 工创智能搬运：初赛静态场地地图 V1

来源：用户提供的 `item.zip/item/item.pdf` 第 3 页（图 1）和第 4 页（表 2）。

## 定义与置信度

- 场地 2.40×2.40 m；中间十字车道宽 0.40 m；四块黄色区域边长 0.45 m；两处蓝色启停区 0.30×0.30 m。这些**尺寸**来自规则。
- 采用场地左下角 `(0,0)`、X 向右、Y 向上的 `map` 坐标定义；分辨率 `0.01 m`。
- 黄色区域按图对称排列：X、Y 都在 `[0.55,1.00]` 或 `[1.40,1.85]` 的组合。**相对位置是从示意图推算，不是现场测量值。**
- 蓝色区：右上 `[2.10,2.40]×[2.10,2.40]`、右下 `[2.10,2.40]×[0,0.30]`。也是按规则图定位；出发选哪一处，由现场抽签确定。
- 左侧暂存区、下侧粗加工区的作业面尺寸为 0.58×0.15 m，分别沿左、下边缘；按图推算为 `x=[0,0.15], y=[0.91,1.49]` 和 `x=[0.91,1.49],y=[0,0.15]`。暂按不允许车辆驶入的区域处理；**需现场核实是否属于实际白色禁行地面或只是功能区示意。**
- 二维码板位置与原料转盘位置有随机性，因此未设置固定姿态或将其画成永久墙体；随机黑色圆柱尚未预标。
- 整张地图周围延伸 0.20 m 显式禁行边框，因此 PGM 实际像素为 `280×280`，YAML 的 image origin 为 `[-0.2,-0.2,0]`，而场地内部的 `(0,0)` 仍是左下角。

## 包含的文件

| 文件 | 用途 |
|---|---|
| `competition_navigation_map.pgm/.yaml` | 静态**合法通行区域**示意；黑色=禁行，白色=可走（包含蓝色启停区），没有动态障碍物 |
| `competition_keepout_mask.pgm/.yaml` | 同样几何的 Nav2 Keepout 掩膜，供以后成本地图禁行过滤器使用 |
| `zones.json` | 结构化保存所有区域坐标、尺寸、已知与待测量说明 |
| `field_preview.svg` / `field_preview.png` | 带颜色和名称的人工核对图，非 ROS 地图 |
| `preview_map.rviz` | 预设 `Fixed Frame=map`、`Map=/map` 的 RViz2 配置 |
| `view_static_map.sh` | 用 `nav2_map_server` 加载地图、激活生命周期并打开 RViz2 |
| `generate_static_map.py` | 只用 Python 标准库重新生成基础 PGM/YAML、SVG/JSON |

## Ubuntu 22.04 + ROS 2 Humble 使用

把整个文件夹复制或解压至：

```
/home/tao/Desktop/ydlidar_ws/maps/competition_v1/
```

首次安装 Map Server：

```bash
sudo apt install -y ros-humble-nav2-map-server ros-humble-rviz2
```

仅查看静态地图（先关闭现有 GMapping，否则可能重复发布 `/map`）：

```bash
bash ~/Desktop/ydlidar_ws/maps/competition_v1/view_static_map.sh
```

另开终端验证：

```bash
source /opt/ros/humble/setup.bash
ros2 topic info /map
ros2 topic echo /map --once --field info
```

查看彩色人工对照图（与运行地图服务器无关）：

```bash
xdg-open ~/Desktop/ydlidar_ws/maps/competition_v1/field_preview.svg
```

## 与 Nav2 的关系

1. 后续 Nav2 的全局、局部成本地图都应正确接入 **Keepout Filter**，从而遵守禁行区域约束。Filter info 约定 `type: 0`、`base: 0.0`、`multiplier: 1.0`，`mask_topic` 与 mask map server 话题保持一致。
2. 必须按机器人真实轮廓配置 `footprint`，并处理通道内安全边距。中央车道只有 400 mm，若机器人实宽 300 mm，居中时理论每边只有 50 mm 余量。只有“路径中心线不进入黑区”并不足以保证机器人本体不碰禁区。
3. 这不是一张**真实激光反射物地图**。颜色边界与平整地面无法由 YDLIDAR X3 直接测得，**不能直接依赖这张地图做 AMCL 激光匹配定位**。定位方案需另行验证。
4. 后续激光雷达检测到的随机圆柱，需要在真实定位和 TF 正确时实时添加到局部或全局 costmap；不能直接当作永久静态背景。
5. 当前只完成第一版“比赛规则对应的合法区域地图”，尚未完成可靠定位、里程计、Nav2、电机通信、二维码视觉识别。

## 赛场任务顺序（仅语义，不代表已规划路线）

抽签确定蓝色启停区 → 沿灰色车道前往二维码板并读取任务 → 根据任务码前往原料区抓取 → 粗加工区与暂存区的搬运/放置 → 返回抽签确定的蓝色启停区。

实际路线需在定位完成、机器人尺寸确认、动态障碍识别后规划。
