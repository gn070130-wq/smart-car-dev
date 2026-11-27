# 2026 smart car

## 项目目录结构

```text
smart2026/
├── .vscode/
├── build/
├── cross_lib/
└── smartCar/
    ├──best_model.ncnn.param
    ├──best_model.ncnn.bin
    ├── code/
    │   ├── car_control.cc
    │   ├── image_proc.cc
    │   ├── line_detect.cc
    │   ├── pid.cc
    │   ├── scene_detect.cc
    │   └── yolov5.cc
    ├── include/
    │   ├── car_control.h
    │   ├── font.h
    │   ├── headfile.h
    │   ├── image_proc.h
    │   ├── json.hpp
    │   ├── line_detect.h
    │   ├── pid.h
    │   ├── scene_detect.h
    │   ├── ww_brushless.h
    │   ├── ww_buzzer.h
    │   ├── ww_camera.h
    │   ├── ww_icm42688.h
    │   ├── ww_ioctl.h
    │   ├── ww_key.h
    │   ├── ww_lcd.h
    │   ├── ww_motor.h
    │   ├── ww_tcp_client.h
    │   ├── ww_timerThread.h
    │   ├── ww_transmission.h
    │   └── ww_vl53l0x.h
    ├── param/
    │   ├── config.json
    │   └── param.hpp
    ├── wuwu_library/
    │   ├── font.cc
    │   ├── ww_brushless.cc
    │   ├── ww_buzzer.cc
    │   ├── ww_camera.cc
    │   ├── ww_icm42688.cc
    │   ├── ww_key.cc
    │   ├── ww_lcd.cc
    │   ├── ww_motor.cc
    │   ├── ww_tcp_client.cc
    │   ├── ww_timerThread.cc
    │   ├── ww_transmission.cc
    │   ├── ww_vl53l0x.cc
    ├── main.cc
├── CMakeLists.txt
├── README.md

main()
├── 初始化所有硬件（Motor/ICM42688/VL53L0X/Camera/Key/Brushless/LCD）
├── 加载 YoloV5Detector（best_model.ncnn，只加载一次）
├── 加载 config.json
├── 启动3个定时线程：
│   ├── IMU线程     5ms：upDataGyro + upDataAcc + 积分姿态角
│   ├── 控制线程   20ms：
│   │   ├── 采图 → 预处理 → 巡线
│   │   ├── 场景检测（十字/斑马/环岛/坡道/障碍）
│   │   ├── 目标板检测（YOLO）→ 设定绕行模式
│   │   ├── 绕行执行（左/右/直行）
│   │   ├── PID转向
│   │   ├── 差速驱动
│   │   └── 图传
│   └── 编码器线程 10ms：update_encoders → 速度估计（可选闭环）
└── 按键线程（阻塞监听）

 * 完整比赛流程：
 *   1. 上电初始化 → 等待按键发车
 *   2. 发车线斑马线（第1次检测，不停车）
 *   3. 巡线行驶：直道/弯道 PID 切换
 *   4. 场景处理：十字路口直行、环岛、路障绕行、坡道减速、目标板识别绕行
 *   5. 终点线斑马线（第2次检测，停车）