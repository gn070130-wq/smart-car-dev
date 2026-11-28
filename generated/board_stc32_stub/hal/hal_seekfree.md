# 逐飞库 HAL 映射（分板）

上板新建 `hal_<board>.c` 实现 `hal.h`。板索引见  
`../../.cursor/skills/smart-car-dev/refs/SEEKFREE_BOARDS.md`。

## 通用

| HAL | 常见调用 |
|-----|----------|
| `hal_adc_inductance` | `adc_init` + `adc_convert` / `adc_mean_filter` |
| `hal_imu_read` | `imu660ra_get_acc/gyro` · `imu963ra_*` |
| `hal_encoder_read` | `encoder_get_count` / `encoder_quad_*` |
| `hal_motor_set` | `pwm_duty`；或 UART→FOC |
| `hal_servo_set` | `pwm_duty` 舵机通道（50Hz） |
| `hal_fan_set` | 独立 PWM / 电调 |
| 节拍 | `pit_init` → `isr_tick` |

**不要**改 `zf_driver_*`；只改 `project/code` + 本 HAL。

## CYT4BB7

- 控制：`hal_*` 全部在 **CM7_0**  
- 视觉：CM7_1 算完写 `vision_to_ctrl_t`，`ipc_send_data` 通知  
- 电机常经 `small_driver_uart_*` 到 CYT2BL3 FOC  
- 详解：`CYT4BB7_DUAL_CORE.md` · 骨架 `generated/board_cyt4bb7_stub/`

## TC264 / TC364 / TC377 / TC387（ADS）

- 工程：`board_libs/seekfree_TC*_Library`  
- 节拍：PIT/STM；电感多通道 ADC；电机 PWM 或 H 桥  
- 卡丁（TC387）：差速限幅更狠，注意散热与电压保护  

## RT1064

- 摄像头 + 电机同板；图像处理勿塞满控制 ISR  
- 大算力任务放主循环或 DMA 完成回调  

## STC32G12K / G144K（Keil C251）

- G12K：资源紧，串口波特率常共用定时器  
- G144K：总钻风、USB 下载、多路高级 PWM+编码器可并存  
- 雁过：激光脉冲与循迹分时，安全互锁必做  

## CYT2BL3

- 作 FOC 从板：只收速度/力矩 UART，不跑视觉  
- 主控（TC264/CYT4BB）经协议下发目标转速  
