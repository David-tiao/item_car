# 工创小车第四阶段：Nav2 Global / Local Costmap Keepout 联调

只测试 **Nav2 Costmap**，不加载雷达、GMapping、规划器、控制器，也不驱动 STM32。

## 用法

1. 先关闭旧版地图预览、Keepout 阶段和 GMapping 等程序。
2. 在安装器成功后运行：

```bash
bash ~/Desktop/ydlidar_ws/maps/competition_v1/costmap_stage4/start_costmap_stage4.sh
```

或者双击桌面的「预览Nav2全局局部代价图」。

## 验收

本阶段从前一阶段继承原地图与掩膜文件。静态 `/map` 使用物理基础地图，**黄色区在原始地图中为自由区**。而两张 Costmap 由 `KeepoutFilter` 把黄色禁行区改为 lethal 100，必须同时验证，才能证明不是原图本来就画黑了。

- `/map`
- `/keepout_filter_mask`
- `/costmap_filter_info`
- `/global_costmap/costmap`（真正 Global Costmap）
- `/local_costmap/costmap`（真正 Local Costmap）

RViz 左侧默认显示 Global Costmap；可关闭其图层，再打开 Local Costmap。蓝色启停区仍通过独立 MarkerArray 显示。

**重要：** 用固定的测试 TF `map->odom->base_link`，机器人固定在 `(1.2,1.2)`；Local Costmap 的 `global_frame` 暂时也为 `map`。此方案仅为无底盘联调，不可作为真车定位。**绝不发布 /cmd_vel**。

`robot_radius: 0.15`、`inflation_radius: 0.10` 均为联调参数，必须用真实底盘外形、车道宽度及误差裕量重新计算。禁止据此推断实车已能够穿行。

## 常见故障

- `map` 有多个发布者：先退出旧地图、Keepout、GMapping/RViz 程序。
- `Costmap` configure/activate 失败：查看 `costmap_stage4/logs/日期时间/costmaps.log`。
- `colcon` 构建失败：查看安装时编译错误；本阶段在独立的 `costmap_stage4/ws` 里编译一个最小的 C++ 适配程序，不影响原 `ydlidar_ws/src`。
- RViz OpenGL 报错时：`LIBGL_ALWAYS_SOFTWARE=1 bash ~/Desktop/ydlidar_ws/maps/competition_v1/costmap_stage4/start_costmap_stage4.sh`

安装器会备份原有 stage4 目录，不修改先前的 V5 静态地图或 Keepout 阶段文件。
