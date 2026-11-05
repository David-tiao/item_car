# 报错日志与修改


# ROS2 静态地图启动问题及报错总结

## 一、项目环境

- 系统：Ubuntu + ROS2 Humble
- 地图服务器：Nav2 Map Server
- 地图可视化：RViz2
- 地图文件：`competition_navigation_map.pgm`
- 地图尺寸：280 × 280
- 地图分辨率：0.01 m/cell
- 启动脚本：`view_static_map.sh`

## 二、主要报错及原因

### 1. map_server 重复启动

**问题表现：**

执行以下命令：

```bash
ps -ef | grep '[m]ap_server'
```

发现系统中同时运行了 3 个 `map_server` 实例。

**原因：**

多次执行地图启动脚本，旧的 `map_server` 进程没有正常退出，导致多个同名节点同时运行。

**解决方法：**

关闭旧的地图服务器进程，确保只运行一个 `map_server`。

后续需要优化启动脚本，实现进程检查和退出清理。

---

### 2. 地图生命周期切换失败

**报错日志：**

```text
[map_server]: Activating
[map_server]: Creating bond
Transitioning failed
```

**原因：**

可能是多个同名 `map_server` 节点同时存在，导致生命周期服务调用发生冲突。

**解决方法：**

清理重复节点，重新启动地图服务器。

检查生命周期状态：

```bash
ros2 lifecycle get /map_server
```

正常状态：

```text
active [3]
```

---

### 3. RViz OpenGL 渲染报错

**报错日志：**

```text
[ERROR] [rviz2]:
active samplers with a different type refer
to the same texture image unit
```

**原因：**

RViz2 使用的 GLSL 着色器与当前 OpenGL 图形环境可能存在兼容性问题。

**解决方法：**

目前 RViz2 能够正常显示地图，因此可以暂时忽略。

如果后续出现黑屏或无法显示地图，可以尝试软件渲染：

```bash
LIBGL_ALWAYS_SOFTWARE=1 rviz2
```

---

### 4. /map 话题无法正常读取

**问题表现：**

执行：

```bash
ros2 topic echo /map --once \
  --qos-durability transient_local \
  --field info
```

终端没有输出地图数据，需要手动按 `Ctrl+C` 退出。

**可能原因：**

- `map_server` 生命周期没有正常完成激活。
- 多个地图服务器重复运行。
- ROS2 话题通信或 QoS 配置存在问题。

**解决方法：**

先清理重复的地图服务器，确认节点处于 `active` 状态。

然后检查：

```bash
ros2 topic info /map -v
```

理想情况下，只保留一个地图发布者：

```text
Publisher count: 1
```

再重新测试地图话题是否能够收到数据。

注意：目前尚未确定 `/map` 无法读取的最终原因。

---

## 三、已经确认正常的功能

1. 静态地图文件可以正常加载。
2. 地图尺寸和分辨率读取正确。
3. RViz2 能够显示静态地图。
4. `/map` 话题已经创建。
5. 曾经检测到 `map_server` 处于 `active [3]` 状态。

## 四、最终总结

目前最主要的问题是：

**多个 map_server 节点重复启动，可能导致生命周期切换异常。**

地图文件本身没有发现明显问题，RViz2 也能够正常显示地图。

下一步重点修改 `view_static_map.sh`：

1. 启动前检查旧的 `map_server` 进程。
2. 确保地图服务器只启动一次。
3. 根据生命周期状态执行配置与激活。
4. 脚本退出时自动清理启动的进程。
5. 验证 `/map` 话题能够正常接收地图数据。

**当前阶段目标：稳定加载并显示静态地图，为后续激光雷达定位和 Nav2 导航做好准备。**
