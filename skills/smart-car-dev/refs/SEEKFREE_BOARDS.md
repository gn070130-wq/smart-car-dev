# 逐飞核心板选型与开源库索引

官方组织：[gitee.com/seekfree](https://gitee.com/seekfree/projects)  
本机镜像（浅克隆、已去 `.git`）：`../board_libs/`  
真机落码流程：`BOARD_TEMPLATES.md` · CYT4BB7 双核专页：`CYT4BB7_DUAL_CORE.md`

## 核心板一览

| 板型 | 内核/要点 | IDE | 官方库 | 典型赛道 |
|------|-----------|-----|--------|----------|
| **CYT4BB7** | 双 Cortex-M7 @~250MHz，IPC | **IAR 9.40.1** | [CYT4BB7_Library](https://gitee.com/seekfree/CYT4BB7_Library) | 轮腿穿越 |
| **CYT2BL3** | M4 @160MHz，FOC 友好 | IAR / 配套工程 | [CYT2BL3_Library](https://gitee.com/seekfree/CYT2BL3_Library) | 无刷双驱、单车协控 |
| **TC264** | AURIX TriCore | **ADS** | [TC264_Library](https://gitee.com/seekfree/TC264_Library) | 疯狂电路、单车、多组电磁/平衡 |
| **TC364** | AURIX 新一代 | ADS | [TC364_Library](https://gitee.com/seekfree/TC364_Library) | 疯狂电路等电磁 |
| **TC377** | AURIX 高性能 | ADS | [TC377_Library](https://gitee.com/seekfree/TC377_Library) | 高速/算力需求 |
| **TC387** | AURIX | ADS | [TC387_Library](https://gitee.com/seekfree/TC387_Library) | 卡丁快跑 |
| **RT1064** | NXP Cortex-M7 | MCUXpresso / MDK | [RT1064_Library](https://gitee.com/seekfree/RT1064_Library) | 智能视觉 |
| **STC32G12K128** | 8051 增强，资源紧 | Keil C251 | [STC32G12K128_Library](https://gitee.com/seekfree/STC32G12K128_Library) | 飞檐走壁（轻量） |
| **STC32G144K246 100Pin** | 资源充裕，总钻风/USB 下载 | Keil C251 | [STC32G144K246_100Pin_Library](https://gitee.com/seekfree/STC32G144K246_100Pin_Library) | 雁过留痕、飞檐 |

产品页示例： [CYT4BB7 核心板](https://www.seekfree.com.cn/产品资料/核心板/英飞凌系列/cyt4bb7核心板/)

## 库目录共识（V3 风格）

```text
Seekfree_*_Opensource_Library/
  project/
    user/          # main / isr —— 少改框架，改逻辑
    code/          # ★ 用户算法（默认改这里）
    mdk|iar|ads/   # IDE 工程
  libraries/
    zf_common/     # 时钟、调试、类型
    zf_driver/     # gpio/adc/pwm/pit/uart/spi/encoder/ipc…
    zf_device/     # imu / 摄像头 / 屏 / tof / 无线
    doc/           # version、GPL
Example/           # 官方例程（母板 Demo）
```

**硬规则**：算法与状态机放 `project/code`（或 `user`）；**不要改** `zf_driver_*` / SDK，除非修确认 bug。

## 通用 API 习惯（跨板近似）

| 能力 | 常见符号 |
|------|----------|
| 周期中断 | `pit_init` / `pit_ms_init` → `isr` 里跑控制 |
| PWM 电机/舵机 | `pwm_init` + `pwm_duty` / `pwm_set_duty` |
| 编码器 | `encoder_quad_init` + `encoder_get_count` |
| ADC 电感 | `adc_init` + `adc_convert` / `adc_mean_filter` |
| IMU | `imu660ra_*` / `imu963ra_*` |
| 摄像头 | `mt9v03x_*` / 总钻风相关（STC32G144 已适配） |
| 延时 | `system_delay_ms` / `zf_delay` |
| 双核通信 | 仅 CYT4BB：`ipc_communicate_init` / `ipc_send_data` |

具体原型以本机 `board_libs/seekfree_<板>/` 头文件为准。

## 赛道 → 推荐板

| 赛道 | 首选板 | 备选 |
|------|--------|------|
| 轮腿穿越 | CYT4BB7（CM7_0 控制 + CM7_1 视觉） | + CYT2BL3 FOC |
| 疯狂电路 | TC264 / TC364 | TC377 |
| 单车定向 | TC264 + CYT2BL3 FOC | TC377 |
| 卡丁快跑 | TC387 | TC377 |
| 智能视觉 | RT1064 | — |
| 雁过留痕 | STC32G144K | — |
| 飞檐走壁 | STC32G12K / G144K | — |
| 蚂蚁搬家 | 规则允许则 STC/RT 等 | 见当年规则 |
| 走马观碑 | **LS2K/Linux**（非本表 MCU） | 见 `LS2K_LINUX_BOARD.md` |
| 智能模型 | **RK3588 NPU** + 下位机 | 见 `RKNN_DEPLOY.md` |

## 上板最小步骤

1. 从 `board_libs/` 或 Gitee 打开对应空工程  
2. 把 `generated/board_seekfree_stub/user/*` 拷进 `project/code`  
3. 按板写 `hal_<board>.c`（见 stub 内 `hal/hal_seekfree.md`）  
4. PIT 调 `isr_tick`；主循环只做屏/按键/日志  
5. 上电默认：电机 0、舵机中位、风机/激光关  

## 本机路径

```text
.cursor/skills/smart-car-dev/board_libs/
  seekfree_CYT4BB7_Library/
  seekfree_TC264_Library/
  seekfree_TC364_Library/
  seekfree_TC387_Library/
  seekfree_TC377_Library/
  seekfree_RT1064_Library/
  seekfree_STC32G12K128_Library/
  seekfree_STC32G144K246_100Pin_Library/
  seekfree_CYT2BL3_Library/
  INDEX.md
```

轮腿实战对照还可看：  
`../../lunhui-chuanyue/code_opensource/届次待核_轮腿穿越__github__lingkonglengfeng__-------/Seekfree_CYT4BB_Opensource_Library/`
