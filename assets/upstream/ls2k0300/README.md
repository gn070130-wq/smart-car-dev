# LS2K300 库与智能车项目第一版

这个仓库保留了完整的 `LS2K300_Library` 目录，目的是方便队友直接下载后查看示例库、对照官方例程，并在同一套环境下继续开发当前智能车项目。

## 仓库用途

- 保留完整示例库，方便队友查阅官方例程
- 保留当前智能车视觉巡线第一版代码
- 避免只拷部分文件后出现依赖缺失、路径错误、无法编译的问题

## 当前项目位置

本次比赛项目的主要代码在下面这个目录：

`Seekfree_LS2K0300_Opensource_Library/project/user/`

其中重点文件如下：

- `main.cpp`
- `vision.hpp`
- `vision.cpp`
- `chassis.hpp`
- `chassis.cpp`

## 当前第一版已实现内容

- 基于灰度图的视觉巡线
- 左右边界提取
- 中线计算与横向误差输出
- 差速底盘控制
- 编码器速度闭环
- IMU 状态与角速度调试显示
- IPS200 图像与调试信息显示

## 当前版本定位

这是一版可以跑通、可以继续迭代的基础版本，主要目标是：

- 先完成基础巡线
- 保持代码模块化
- 方便后续继续调 PID、加状态机、加特殊元素识别

当前仓库刻意保留整套库和示例，不做极限精简，主要是为了团队协作方便。

## 编译相关

主工程目录：

`Seekfree_LS2K0300_Opensource_Library/project/user`

常见编译输出目录：

`Seekfree_LS2K0300_Opensource_Library/project/out`

这些编译产物已经通过 `.gitignore` 忽略，不应该提交到仓库里。

## 给队友的说明

如果只是想看你们自己的项目代码，直接进入：

`Seekfree_LS2K0300_Opensource_Library/project/user/`

如果想查官方示例，请看：

`Example/`

如果想看底层驱动和库实现，请看：

- `Seekfree_LS2K0300_Opensource_Library/libraries/`
- `Example/...` 中对应的例程


2026/5/6 
重写了PID，视觉，抛弃了一大块答辩的ips200的显示函数，采用上位机显示
