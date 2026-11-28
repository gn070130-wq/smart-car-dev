# CYT4BB7 双核工程指南（逐飞）

官方库：[gitee.com/seekfree/CYT4BB7_Library](https://gitee.com/seekfree/CYT4BB7_Library)  
产品资料：[逐飞 CYT4BB7 核心板](https://www.seekfree.com.cn/产品资料/核心板/英飞凌系列/cyt4bb7核心板/)  
IDE：**IAR 9.40.1**（与库注释一致）  
本机：`../board_libs/seekfree_CYT4BB7_Library/` + 轮腿对照仓内完整 `Seekfree_CYT4BB_Opensource_Library`

## 芯片与双核

- Infineon TRAVEO T2G：**双 Cortex-M7**（CM7_0 / CM7_1）  
- 典型主频配置示例：`clock_init(SYSTEM_CLOCK_250M)`  
- 核间通信：`zf_driver_ipc` → `ipc_communicate_init` / `ipc_send_data`  
- 另可挂 **CYT2BL3** 作无刷 FOC 从板（UART 收速度命令）

## 推荐职责划分

| 核 | 文件入口 | 职责 | 节拍建议 |
|----|----------|------|----------|
| **CM7_0** | `project/user/main_cm7_0.c` + `cm7_0_isr.c` | IMU、平衡/腿、电机舵机、科目 FSM、安全 | PIT **1–5ms**，平衡优先 1ms |
| **CM7_1** | `project/user/main_cm7_1.c` + `cm7_1_isr.c` | 摄像头、IPM/中线、元素识别、图传 | 帧就绪处理；**禁止**拖死 CM7_0 |

原则：视觉结果经 **IPC 或共享内存** 下发 `err / element / flag`；控制环只读最新快照，带超时。

## 工程树（逐飞库）

```text
Seekfree_CYT4BB_Opensource_Library/
  project/
    user/
      main_cm7_0.c / main_cm7_1.c
      cm7_0_isr.c  / cm7_1_isr.c
    code/                 # ★ 算法：pid / control / state_machine / filter…
    iar/                  # IAR 工程与双核调试配置
  libraries/
    zf_driver/            # 含 zf_driver_ipc.*
    zf_device/            # imu963ra 等
    zf_common/
```

用户代码默认进 `project/code/`（见文件夹内「本文件夹作用」说明）。

## 启动顺序（CM7_0 实战共识）

对照开源（lingkonglengfeng 等）：

1. `clock_init` → `debug_init`  
2. GPIO / UART / 外设（IMU、FOC UART、腿舵机 PWM…）  
3. 滤波与 `control_init` / `state_machine_init`  
4. **最后** `pit_init(PIT_CHx, period_us)` —— 先开 PIT 再 init 控制易空指针/未赋 KP  

示例（节选自对照仓，周期按队标定）：

```c
clock_init(SYSTEM_CLOCK_250M);
debug_init();
imu963ra_init();
/* pwm_init 腿舵机 / 风机… */
control_init();
state_machine_init();
pit_init(PIT_CH0, 1000);  /* 单位以库注释为准，常为 us→对应 1ms 级 */
```

## IPC 用法要点

```c
/* zf_driver_ipc.h */
void  ipc_communicate_init(ipc_port_enum ipc_port, ipc_callback_function cb);
uint8 ipc_send_data(uint32 send_data);
```

建议自建结构化共享：

```c
typedef struct {
  float err;          /* 中线误差 */
  uint8_t element;    /* 元素枚举 */
  uint8_t valid;
  uint32_t stamp_ms;
} vision_to_ctrl_t;
```

- CM7_1：算完写共享 + `ipc_send_data` 通知  
- CM7_0：回调或轮询取快照；`stamp` 超时则 `err=0` 并降速  

## HAL 映射（CYT4BB）

| 抽象 `hal.h` | 逐飞 CYT4BB |
|--------------|-------------|
| `hal_imu_read` | `imu963ra_get_*` / `imu660ra_*` |
| `hal_motor_set` | `pwm_duty` 或 UART→FOC（`small_driver_uart_*`） |
| `hal_servo_set` | `pwm_init(TCPWM_CHxx_Pyy, 50, …)` + duty |
| `hal_encoder_read` | `encoder_get_count` |
| `hal_adc_inductance` | `adc_convert`（若电磁辅循迹） |
| 节拍 | `pit_init` → `cm7_0_isr` 调控制 |

Host 可编译双核示意：`generated/board_cyt4bb7_stub/`

## 安全（双核）

- 倾角超限：CM7_0 立刻 PWM/FOC 清零，无视视觉  
- CM7_1 看门狗：图像超时只降级，不阻塞 PIT  
- 上电双核均默认执行器安全态  

## 与赛道 Skill

- 轮腿：`../../lunhui-chuanyue/refs/FULL_SUMMARY.md`  
- 平衡环：`IMU_INS_PLAYBOOK.md`  
- 落码总流程：`COMPILE_READY_WORKFLOW.md` → 再迁 IAR  
