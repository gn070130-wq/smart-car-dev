# 真机工程模板（逐飞 / ADS）

落码时：算法放 `user/code`，驱动用库函数；**禁止改库核心**除非修 bug。

板型总表 → **`SEEKFREE_BOARDS.md`**  
CYT4BB7 双核 → **`CYT4BB7_DUAL_CORE.md`**  
龙芯 LS2K → **`LS2K_LINUX_BOARD.md`**（走马；不改 MCU stub 结构）  
RK3588/RKNN → **`RKNN_DEPLOY.md`**（智能模型）  
本机官方库 → **`../board_libs/INDEX.md`**

## 目录约定

### A. 单核通用（对照 `generated/board_*_stub/`）

```text
board_seekfree_stub/   # 通用
board_tc264_stub/      # TC264 + ADS
board_stc32_stub/      # STC32（雁过/飞檐）
board_rt1064_stub/     # RT1064（智能视觉）
  app/main.c, isr_tick.c
  user/          # track / element / control / safety
  hal/hal.h, hal_host_stub.c, hal_seekfree.md
```
结构相同；差别在 README 指向的 IDE/库目录。

### B. CYT4BB7 双核（对照 `generated/board_cyt4bb7_stub/`）

```text
board_cyt4bb7_stub/
  cm7_0/         # 控制核：平衡/电机 + 读视觉快照
  cm7_1/         # 视觉核：算 err/元素 + IPC 通知
  shared/        # vision_to_ctrl 结构
  user/          # 可与单核 stub 共用的控制算法
  hal/
```

## 主控映射（摘要）

| 赛道 | 板 | 库目录（board_libs） |
|------|-----|----------------------|
| 轮腿 | CYT4BB7 | `seekfree_CYT4BB7_Library` |
| 电磁/单车 | TC264/TC364 | `seekfree_TC264_Library` / `TC364` |
| 卡丁 | TC387 | `seekfree_TC387_Library` |
| 视觉 | RT1064 | `seekfree_RT1064_Library` |
| 雁过/飞檐 | STC32G144 / G12K | `seekfree_STC32G144…` / `STC32G12K…` |
| 无刷 FOC | CYT2BL3 | `seekfree_CYT2BL3_Library` |
| ADS 系 | TC26x/36x/37x/38x | 用 ADS 打开对应 Library 工程 |

## 上板替换步骤

1. 打开 `board_libs/seekfree_<板>_Library` 空工程（或赛道 RECOMMENDED 仓）  
2. 拷贝 `user/*.c`（双核则拆到 cm7_0 / cm7_1）  
3. 实现 `hal_<board>.c`：对接 `pwm` / `adc` / `imu` / `encoder` / `ipc`  
4. 节拍放 PIT；主循环只做显示与低优先级  
5. 上电默认：电机 0、舵机中位、风机关、激光关  

## 与 COMPILE_READY 关系

- 先在 `generated/board_*_stub` 用 stub HAL **本地编过**  
- 再迁 IAR/ADS/Keil；对照赛道 `code_opensource/RECOMMENDED.md`  
