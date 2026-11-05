# 工创比赛 Keepout 数据服务阶段

此目录部署在原来的 `~/Desktop/ydlidar_ws/maps/competition_v1/keepout_stage`，不覆盖已有 V5 静态地图、圆盘和色环彩色预览。

## 启动前

关闭先前开启的 `view_static_map.sh`、GMapping、地图预览 RViz2 等程序。**不要同时运行多个 `/map` 发布者。**

## 启动

```bash
bash ~/Desktop/ydlidar_ws/maps/competition_v1/keepout_stage/start_keepout_stage.sh
```

启动程序会检查 ROS 节点冲突，分别激活三个生命周期节点，并通过真正订阅验证：

- `/map` —— 旧有比赛规则静态地图预览（OccupancyGrid）
- `/keepout_filter_mask` —— Nav2 专用禁行掩膜（OccupancyGrid）
- `/costmap_filter_info` —— Nav2 过滤信息（CostmapFilterInfo，`type=0`）
- `/competition_start_zones` —— RViz 蓝色启停区 MarkerArray

禁行掩膜由已存在的 `competition_keepout_mask.pgm` 派生，不改原文件，且把未知值（包含场地外）一律变成禁止。地图原点、分辨率保持不变。

RViz 默认显示 `/map` 和蓝色启停区。若需单独看禁行掩膜，先关闭 `Competition Static Map`，再勾选 `Keepout Mask (toggle on, Map off)`。

## 未来 Nav2 集成

`keepout_costmaps_snippet.yaml` 是全局和局部代价地图的 **合并示例**，不可直接覆盖已有完整 `nav2_params.yaml`。导航时还需要机器人实时位姿、TF、真实车体轮廓、代价地图膨胀、控制器和电机接口。此阶段**未启动完整 Nav2 规划/控制**，也**未向 STM32 发送运动指令**。

如图像显示异常可以尝试：

```bash
LIBGL_ALWAYS_SOFTWARE=1 bash ~/Desktop/ydlidar_ws/maps/competition_v1/keepout_stage/start_keepout_stage.sh
```

关闭 RViz2 会结束本次启动的三个服务进程和蓝色标记节点；不会尝试强制杀掉其他进程。日志位于 `keepout_stage/logs/`。
