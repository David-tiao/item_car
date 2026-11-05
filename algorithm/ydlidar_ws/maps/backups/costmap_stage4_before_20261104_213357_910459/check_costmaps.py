#!/usr/bin/env python3
"""验收：静态原图黄区可通行，但全局/局部代价图黄区必须不可通行。"""
import sys, time
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, DurabilityPolicy, ReliabilityPolicy
from nav_msgs.msg import OccupancyGrid

TOPICS={
    '/map':OccupancyGrid,
    '/keepout_filter_mask':OccupancyGrid,
    '/global_costmap/costmap':OccupancyGrid,
    '/local_costmap/costmap':OccupancyGrid,
}
qos=QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL,
               reliability=ReliabilityPolicy.RELIABLE)
rclpy.init()
node=Node('competition_stage4_costmap_verifier')
msgs={}
subs=[]
for topic,ty in TOPICS.items():
    subs.append(node.create_subscription(ty,topic,lambda msg,t=topic:msgs.__setitem__(t,msg),qos))

def at(msg,x,y):
    info=msg.info
    col=int((x-info.origin.position.x)/info.resolution)
    row=int((y-info.origin.position.y)/info.resolution)
    if not (0<=col<info.width and 0<=row<info.height):
        return None
    return int(msg.data[row*info.width+col])

def evaluate():
    if len(msgs)<len(TOPICS):
        return False, f'缺失话题: {set(TOPICS)-set(msgs)}'
    if at(msgs['/map'],0.75,0.75)!=0:
        return False, '物理地图的黄色区本应可通行，请确认使用 competition_navigation_map.yaml'
    if at(msgs['/keepout_filter_mask'],0.75,0.75)!=100:
        return False, '禁行掩膜黄色区域不是 100'
    for name in ('global_costmap','local_costmap'):
        msg=msgs[f'/{name}/costmap']
        vals={
            '黄色禁行':at(msg,0.75,0.75),
            '灰色十字中心':at(msg,1.20,1.20),
            '蓝色启停区':at(msg,2.25,2.25),
            '场地外':at(msg,-0.10,1.20),
        }
        # 黄色和场地外必须实际达到 lethal；蓝区仍可通行。
        if vals['黄色禁行']!=100 or vals['场地外']!=100:
            return False, f'{name} 的黄色/场外禁行未生效: {vals}'
        if vals['灰色十字中心'] is None or vals['灰色十字中心']>=99:
            return False, f'{name} 的中心车道不通行: {vals}'
        if vals['蓝色启停区'] is None or vals['蓝色启停区']>=99:
            return False, f'{name} 的蓝区不通行: {vals}'
    return True,'全部位置符合要求'

try:
    start=time.monotonic()
    success=False
    last_reason='尚未接收数据'
    while time.monotonic()-start<30:
        rclpy.spin_once(node,timeout_sec=0.4)
        success,last_reason=evaluate()
        if success:
            break
    for topic in TOPICS:
        print(f'{topic}: 发布者={len(node.get_publishers_info_by_topic(topic))}, 收到数据={topic in msgs}',flush=True)
    if not success:
        raise RuntimeError(last_reason)
    for name in ('global_costmap','local_costmap'):
        msg=msgs[f'/{name}/costmap']
        print(f'{name}: 禁行黄色={at(msg,.75,.75)}, 中心={at(msg,1.2,1.2)}, 蓝区={at(msg,2.25,2.25)}, 场外={at(msg,-.1,1.2)}',flush=True)
    print('✅ 第四阶段通过：原始物理地图黄区自由，但两张真正的 Nav2 Costmap 均将其变成禁行。',flush=True)
except Exception as err:
    print('❌ 第四阶段验收失败:',err,file=sys.stderr,flush=True)
    sys.exit(1)
finally:
    node.destroy_node()
    rclpy.shutdown()
