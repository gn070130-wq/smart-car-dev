# 智能车软件通用骨架（可粘贴）

用于「从零搭建」时的默认目录与主循环。按赛道改名模块，但保持分层。

## 推荐目录树

```text
firmware/
  app/
    main.c              # 主循环、模式、看门狗
    mode.h              # IDLE/CALIB/RUN/FAULT
  perception/
    sense.h / sense.c   # 电感/图像/IMU 采样与预处理
    track.h / track.c   # 中线/边线/目标估计
  planning/
    element.h / .c      # 元素/任务状态机
    speed_plan.h / .c   # 目标速度表
  control/
    steer.h / .c        # 转向环（PD/纯跟踪等）
    speed.h / .c        # 速度环（增量 PI）
    limit.h / .c        # 统一限幅
  actuators/
    motor.h / .c
    servo.h / .c
    safety.h / .c       # 急停、互锁、超时安全态
  bsp/                  # 板级：PWM/ADC/UART/相机
    tof_vl53.*          # TOF 测距（见 MODULES_BSP.md）
    encoder.*           # 编码器
    ir_board.*          # 红外灯板/灰度
    imu_gyro.*          # 陀螺/IMU（见 IMU_INS_PLAYBOOK.md）
    adc_sensor.*        # 电感/电池 ADC
  lib/                  # 厂商/Seekfree（尽量只读）
  cfg/
    params.h            # 可调参数（PID、阈值、超时）
  log/
    telem.h / .c        # 最小遥测字段
```

## 主循环骨架（伪代码）

```c
void app_main(void) {
  bsp_init();
  params_load_defaults();
  safety_enter_safe();   // 上电默认安全
  mode = MODE_IDLE;

  for (;;) {
    uint32_t now = time_ms();
    safety_kick(now);

    sense_update();                    // 非阻塞采样
    if (image_too_old(now)) fault_vision_stale();

    switch (mode) {
      case MODE_IDLE:  /* 等待启动 */ break;
      case MODE_CALIB: calib_step(); break;
      case MODE_RUN:
        track_estimate();
        element_update();              // 状态机
        float steer = steer_control(track_err);
        float spd   = speed_control(speed_plan_target());
        actuators_apply(steer, spd);   // 内部限幅
        break;
      case MODE_FAULT:
        safety_enter_safe();
        break;
    }

    telem_log(now);
    // 控制周期固定；推理放队列/另一任务
  }
}
```

## 模块接口约定

| 模块 | 输入 | 输出 |
|------|------|------|
| sense | 硬件 | raw + 时间戳 |
| track | sense | err/中线/置信度/年龄 |
| element | track+里程 | 状态、目标速度、特殊动作 |
| steer/speed | err/目标 | 执行量（已限幅） |
| safety | 超时/电压/按键 | SAFE/FAULT |

## 最小测试顺序

1. 编译下载 → 心跳日志  
2. 开环点动电机 → 急停有效  
3. 打印传感器 raw  
4. 低速转向闭环  
5. 速度闭环  
6. 状态机空跑  
7. 再加赛场元素/视觉推理  
