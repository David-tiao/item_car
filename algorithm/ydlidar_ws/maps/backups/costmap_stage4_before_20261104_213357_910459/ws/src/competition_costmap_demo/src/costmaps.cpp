#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "nav2_costmap_2d/costmap_2d_ros.hpp"

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  // 单独构造两个 Nav2 Costmap2DROS 生命周期节点。
  // Humble 自带的 nav2_costmap_2d 可执行程序固定节点命名为 costmap，
  // 这里使用官方 C++ 类构造，确保有两个独立的标准命名空间。
  auto global = std::make_shared<nav2_costmap_2d::Costmap2DROS>(
      "global_costmap", "/", "global_costmap");
  auto local = std::make_shared<nav2_costmap_2d::Costmap2DROS>(
      "local_costmap", "/", "local_costmap");
  rclcpp::executors::MultiThreadedExecutor executor(rclcpp::ExecutorOptions(), 4);
  executor.add_node(global->get_node_base_interface());
  executor.add_node(local->get_node_base_interface());
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
