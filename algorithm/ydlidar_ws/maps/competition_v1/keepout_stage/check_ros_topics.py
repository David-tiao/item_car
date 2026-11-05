#!/usr/bin/env python3
"""检查单发布者、静态地图、Nav2 Keepout Mask、Info 三条话题的真实可接收性。"""
import sys
import time
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, DurabilityPolicy, ReliabilityPolicy
from nav_msgs.msg import OccupancyGrid
from nav2_msgs.msg import CostmapFilterInfo

qos = QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL,
                 reliability=ReliabilityPolicy.RELIABLE)
rclpy.init()
node = Node('competition_keepout_validation')
messages = {}
subs = []
try:
    topics = {
        '/map': OccupancyGrid,
        '/keepout_filter_mask': OccupancyGrid,
        '/costmap_filter_info': CostmapFilterInfo,
    }
    for topic, msg_type in topics.items():
        subs.append(node.create_subscription(
            msg_type, topic, lambda msg, t=topic: messages.__setitem__(t, msg), qos))
    deadline = time.monotonic() + 14.0
    while time.monotonic() < deadline and len(messages) < 3:
        rclpy.spin_once(node, timeout_sec=0.3)
    for topic in topics:
        pubs = node.get_publishers_info_by_topic(topic)
        print(f'{topic}: 发布者 {len(pubs)} 个，接收消息 = {topic in messages}')
        if len(pubs) != 1 or topic not in messages:
            raise RuntimeError(f'{topic} 发布者重复或收不到消息')
    m = messages['/keepout_filter_mask']
    if m.info.width != 280 or m.info.height != 280:
        raise RuntimeError('Keepout 掩膜尺寸异常')
    if m.header.frame_id != 'map' or abs(m.info.resolution - 0.01) > 1e-6:
        raise RuntimeError('Keepout 坐标系或分辨率异常')
    if abs(m.info.origin.position.x + 0.2) > 1e-5 or abs(m.info.origin.position.y + 0.2) > 1e-5:
        raise RuntimeError('Keepout 原点不正确')
    def check(x,y,expected,label):
        col=int((x - m.info.origin.position.x)/m.info.resolution)
        row=int((y - m.info.origin.position.y)/m.info.resolution)
        value=m.data[row*m.info.width+col]
        print(f'  {label}: 占据值={value}, 预期={expected}')
        if value != expected:
            raise RuntimeError(f'{label} 禁行逻辑不正确')
    check(1.2,1.2,0,'中心车道')
    check(0.75,0.75,100,'左下黄色区')
    check(2.25,2.25,0,'蓝色启停区')
    check(-0.1,1.2,100,'场地外部')
    info=messages['/costmap_filter_info']
    if info.type != 0 or info.filter_mask_topic != '/keepout_filter_mask':
        raise RuntimeError('CostmapFilterInfo 的类型或掩膜话题不正确')
    print('Keepout 信息 type=0，mask_topic=/keepout_filter_mask')
    print('通过：静态地图 + Nav2 Keepout 数据服务链路')
except Exception as exc:
    print('检查失败:', exc, file=sys.stderr)
    sys.exit(1)
finally:
    node.destroy_node()
    rclpy.shutdown()
