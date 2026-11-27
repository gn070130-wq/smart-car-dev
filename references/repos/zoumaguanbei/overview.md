# ZouMaGuanBei 开源工程学习入口

源仓库：[CoConutMilkSoy/ZouMaGuanBei](https://github.com/CoConutMilkSoy/ZouMaGuanBei)。本次读取提交 `69014373f172a9442f148a853cfb282f454395a9`，获取日期2026-11-27。README自述为厦门理工学院“索罗芬-阳光青提”队项目，与报告库37对应；没有通过独立比赛资料核验其名次，也不把当前提交等同参赛现场版本。

## 包含什么

- [algorithms.md](algorithms.md)：真实调用链、视觉、ROI、控制与状态含义。
- [porting-and-debug.md](porting-and-debug.md)：该提交已核实的缺项、耦合和调车定位路径。
- [build-and-tools.md](build-and-tools.md)：构建依赖、硬件适配、通信和网页工具。
- [manifest.json](manifest.json)：所选110个文本文件的Git blob、SHA256、大小、固定提交链接与覆盖边界。
- [symbols.json](symbols.json)：从源码提取的函数定义候选、文件与行号。它是正则索引，不是C++完整语法/调用图解析器。

GitHub版本移除了本机源码快照路径，可用固定链接或另取同提交源码，并通过检索工具的 --repo 参数指定本地目录。学习包只携带原创解析和定位数据，避免把内核、工具链、巨大采样表和第三方代码混入技能上下文。分析时未执行仓库安装、构建、部署、驱动或电机命令。

## 从实际入口理解架构

```text
main/main.cpp::main
  ├─ 参数/触摸/GPIO/PWM初始化
  ├─ fork：父进程waitpid，子进程注册退出清理
  ├─ 相机MMAP、model_load("75ttv1")、菜单
  └─ start.cpp::main_run
      ├─ 周期线程：pit_callback_speed → 编码器 → speed_control → PWM
      └─ 逐帧：取图 → 边界 → 元素/参考边 → 中线 → 目标ROI/分类
               → 预瞄误差/目标角速度 → 差速目标 → 更新保持/冷却计数
```

这里的轮速闭环独立于视觉，方向与角速度误差计算却主要在视觉循环内。不要描述成“三个环都由5ms任务运行”。源码有姿态四元数、串口舵机、其他采图模式及调试服务，不代表比赛主循环全部调用。

父进程等待子进程退出后清理，不含持续心跳超时检测，不能等同硬件看门狗或活锁检测器。报告37描述的硬件看门狗也不能仅由这个fork实现证明。

## 按问题直接找文件

| 问题 | 文件与主要符号 |
|---|---|
| 开机、模型加载、运行模式 | `project/main/main.cpp::main` |
| 正常跑车顺序、绕行/冷却/退出 | `project/src/start.cpp::main_run` |
| 二值化、固定采图缓冲、映射表 | `project/src/camera.cpp::get_mmap_image`、`ImagePerspective*_Color_Init` |
| 法线中线、前瞻和赛道元素 | `project/src/find_road.cpp` |
| 目标近边拟合、ROI方向 | `find_barrier_accurate`、`camera.cpp::cut_image` |
| 模型配置、通道、输出含义 | `classifier.cpp::model_load/model_identify`、`project/models/*/*.json` |
| 轮速增量PID、角速度PD | `control.cpp::speed_control/error_compute` |
| 周期为何漂移 | `libraries/zf_driver/zf_driver_pit.cpp::Pit_timer::timerLoop` |
| PWM、方向、编码器实际接口 | `pwm.cpp`、`my_gpio.cpp`、`zf_driver_encoder.cpp`、相关头文件 |
| 联机轮速调试 | `udp.cpp`、`uart.cpp`、`project/tools` |
| 采集数据集、相机设置、模型测试 | `project/web_service` |

使用 `lookup_open_source.py --query 名称` 查固定版本链接，或加 `--repo 本机目录 --context 3` 查看已校验文件的上下文。先比对实际提交/哈希再引用行号。需要整体重构时沿此调用链拆分，不能按README功能列表重写后声称行为等价。

## 与53份报告如何结合

报告37帮助理解设计意图；此处源码帮助确认当前实现。相同队伍不保证文档与源码时点一致。例如当前精定位是红条近边候选、确定性假设内点筛选与直线拟合，不应沿用其他队伍的“旋转矩形/洪水填充”描述来代替。

使用规则：先读当前用户工程，再读相关报告和开源函数，借鉴局部机制；保留当前项目接口、单位、时序与标定。没有对应板卡、SDK、模型和场地数据时，只能完成静态/离线验证，不能宣称整车已经跑通。
