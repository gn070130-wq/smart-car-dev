# 走马观碑

## 项目简介

本项目是**第 21 届全国大学生智能汽车竞赛“走马观碑”组国冠**——厦门理工学院**索罗芬-阳光青提队**的开源竞赛项目代码。

项目运行于以龙芯 2K0300 为核心的智能车平台，主要完成摄像头采集、图像预处理与逆透视、赛道边界提取、特殊元素识别、视觉模型推理、速度与方向控制、编码器/IMU/串口通信，以及面向调试的数据集采集、摄像头配置和模型测试网页服务。


## 系统组成

```text
摄像头 / 传感器
        │
        ▼
  图像采集与预处理 ──► 赛道/障碍物/斑马线/环岛识别
        │                         │
        └──────────────► 控制决策与 PID 调速
                                  │
                 电机、舵机、LED、屏幕、串口/UDP
```

视觉模型采用 ncnn 推理，图像处理依赖 OpenCV；程序通过 V4L2、Linux GPIO/PWM、串口和 UDP 等接口与目标硬件交互。

## 目录结构

```text
.
├── project/                    # 竞赛应用程序（核心目录）
│   ├── cmake/                  # CMake、交叉编译和构建/部署脚本
│   ├── include/                # 应用程序头文件及模型配置依赖
│   ├── main/                   # 程序入口
│   ├── models/                 # ncnn 模型配置及模型文件放置目录
│   ├── src/                    # 核心 C/C++ 源文件
│   ├── test/                   # 算法/数据处理测试和生成脚本
│   ├── tools/                  # 调试、监控、诊断和赛道设计工具
│   ├── web_service/            # 网页调试与数据集服务
│   └── out/                    # project 专用构建输出目录
├── libraries/                  # 随仓库提供的第三方库和逐飞相关库
│   ├── opencv/                 # OpenCV 及其交叉编译库
│   ├── ncnn/                   # ncnn 推理库
│   ├── libjpeg/                # JPEG/TurboJPEG
│   ├── giflib/                 # GIF 解码
│   ├── ncurses-6.5/            # 终端界面依赖
│   ├── zf_common/              # 逐飞公共组件
│   ├── zf_components/          # 逐飞组件层
│   ├── zf_device/              # 逐飞设备驱动封装
│   └── zf_driver/              # 逐飞底层驱动封装
├── linux4.19/                  # 目标平台 Linux 4.19 内核源码/构建内容
├── toolchain_X86_LoongArch/    # x86 主机上的 LoongArch 交叉编译工具链
├── build/                      # CMake 外部构建目录
└── .vscode/                    # 编辑器配置
```

## `project/` 目录详解

`project/` 是本项目真正的应用工程目录。CMake 从 `project/cmake/` 启动，自动收集 `main/`、`src/`、`web_service/src/` 以及逐飞库源文件，最终生成名为 `project` 的可执行程序。

| 目录 | 功能 |
|---|---|
| `cmake/` | 定义交叉编译器、LoongArch 编译选项、OpenCV/ncnn/libjpeg/giflib 路径、源文件收集和链接关系；`build.sh` 还负责构建后向目标机传输程序。 |
| `include/` | 应用模块公共接口、硬件参数、图像尺寸和数据结构。`json.hpp` 为单头文件 JSON 库，`rotate_array.h` 保存旋转裁剪使用的采样表。 |
| `main/` | 主程序入口、全局运行状态、信号处理和资源清理。 |
| `models/` | ncnn 模型描述 JSON。对应的 `.param`/`.bin` 文件需按 JSON 中的文件名放在同一目录；当前包含道路标志和障碍物分类模型配置。 |
| `src/` | 车辆运行所需的采集、识别、控制、显示、通信和硬件抽象实现，详见下文。 |
| `test/` | 旋转映射表、图像/算法辅助测试脚本和生成结果。 |
| `tools/` | 摄像头 V4L2 诊断、UDP 监控/桥接、赛道设计器等开发调试工具。 |
| `web_service/` | 运行在目标机上的 HTTP 调试服务：服务选择、数据集拍摄、摄像头参数调整和模型测试。 |
| `out/` | `cmake/build.sh` 使用的构建输出目录；内容可删除并重新生成。 |

### `project/src/` 源文件功能

| 文件 | 主要功能 |
|---|---|
| `adc.cpp` | 读取 Linux IIO ADC 原始值，并换算电池电压。 |
| `camera.cpp` | 摄像头自动发现与初始化、V4L2/MMAP 采集、YUYV/JPEG 图像处理、阈值/大津二值化、逆透视、旋转裁剪、曝光和画面参数配置持久化。 |
| `classifier.cpp` | ncnn 模型配置解析、模型加载/切换、图像分类推理、类别/动作映射，以及障碍物照片缓存和批量保存。 |
| `control.cpp` | 根据赛道误差和编码器反馈进行速度控制，包含 PID/滤波、左右轮差速、停车和控制状态复位。 |
| `cst816.cpp` | CST816 触摸芯片的 I2C 通信、初始化、读写和测试。 |
| `display.cpp` | LCD/帧缓冲显示、菜单界面、像素/图像输出、调试信息、GIF 播放和视频定时播放。 |
| `find_road.cpp` | 从逆透视图像中提取左右边界和中线，判断直道、弯道、环岛、斑马线、坡道及障碍物，并计算转向误差、角度和相关状态。 |
| `imu.cpp` | 读取/处理 IMU 加速度计和陀螺仪数据，更新姿态四元数并输出欧拉角。 |
| `keyboard.cpp` | 键盘 GPIO、Linux 输入和 CST816 触摸按键扫描。 |
| `led.cpp` | LED 初始化、状态控制、定时闪烁和清理；根据车辆状态提示转向、障碍物等信息。 |
| `my_gpio.cpp` | 基于 Linux GPIO sysfs 的 GPIO 打开、方向设置、读写和资源管理。 |
| `my_timeer.cpp` | 提供 CPU 单调时间和墙上时间的纳秒级读取函数。 |
| `pwm.cpp` | 基于 Linux PWM sysfs 的 PWM 初始化、频率/占空比/极性设置和清理。 |
| `setting.cpp` | 参数菜单、运行配置读写、速度/PID/摄像头等参数调整和保存恢复。 |
| `start.cpp` | 车辆主运行流程和全局状态：初始化传感器/模型，循环采图、找线、识别和控制，并处理起跑、赛道元素及结束清理。 |
| `temperature.cpp` | 从系统 thermal 节点读取 CPU 温度，提供毫摄氏度和摄氏度接口。 |
| `uart.cpp` | 普通串口初始化、数据发送、编码器/控制状态传输和定时回调。 |
| `uart_servo_linux.cpp` | Linux 串口舵机协议封装，包括校验、扭矩、角度、速度和动作时间控制。 |
| `udp.cpp` | UDP 初始化、大小端数据编解码、遥测发送、控制命令接收和定时通信。 |

### `project/main/` 与 `project/web_service/` 源文件

- `main/main.cpp`：应用程序入口，建立硬件与运行环境、处理退出信号，并进入菜单或车辆运行流程。
- `web_service/src/web_service_main.cpp`：HTTP 服务主程序、路由/API、服务生命周期和退出时的线程回收。
- `web_service/src/dataset_test_service.cpp`：数据集拍摄、文件夹识别、模型管理和可取消测试线程。
- `web_service/src/home_page.cpp`：网页服务选择首页。
- `web_service/src/capture_page.cpp`：数据集拍摄页面。
- `web_service/src/camera_settings_page.cpp`：摄像头曝光和画面参数配置页面。
- `web_service/src/model_test_page.cpp`：模型选择、切换和识别测试页面。
- `web_service/src/web_common_style.cpp`：各网页共享的 CSS 样式。


## 构建流程

### 环境要求

- Linux x86 主机；
- CMake 3.10 或更高版本、Make；
- 仓库内 `toolchain_X86_LoongArch/` 可用的 `loongarch64-linux-gnu-gcc/g++`；
- 仓库内 OpenCV、ncnn、libjpeg-turbo、giflib 和逐飞库；
- 目标机运行时具备对应动态库、摄像头、GPIO/PWM、串口等设备。



### 一键构建与部署

```bash
cd project/cmake
./build.sh
```

该脚本会清理 `project/out/`、执行 CMake/Make，然后通过 `scp` 将生成的 `project` 发送到目标机 `/home/root/`。脚本默认目标地址为 `192.168.8.50`，并会保存最近使用的地址；首次使用或目标地址变化时请先检查并修改脚本中的 IP。

## 运行与调试提示

- 目标机上的模型、配置和动态库路径应与程序约定一致；摄像头配置默认保存到 `/home/root/configs/camera_config.json`。
- 摄像头初始化会自动寻找同时支持视频采集、流式传输和 YUYV 的 `/dev/video*` 节点，并排除不适合直接采集的媒体管线节点。
- `project/tools/camera_v4l2_diagnose.sh` 可用于摄像头节点和 V4L2 能力诊断；`udp_monitor.py`、`udp_bridge.py` 用于通信调试。
- 网页服务用于调试和数据采集，不是车辆闭环运行的必要组件；数据集测试线程支持取消和安全回收。

## 开源说明

本仓库面向学习、研究和智能车竞赛技术交流开源。仓库内第三方库、逐飞库、Linux 内核、模型及其配套文件分别遵循各自许可证或原始项目要求；使用和再发布时请同时遵守相应许可与竞赛规则。
