# 外设多模块代码库（TOF / 编码器 / 红外灯板等）

按赛题组装时：复制到 `bsp/` 或 `drivers/`，把 `hal_*` 换成你板级库（Seekfree / 厂商库 / ROS 驱动）。

统一约定：
- 距离单位：**mm**；速度：**mm/s** 或 **count/s**（在注释写清）
- 每个模块带 `stamp_ms`；控制环发现超时 → 安全处理
- 所有输出限幅；初始化失败返回非 0

---

## 目录建议

```text
bsp/
  tof_vl53.h / tof_vl53.c      # TOF 测距
  encoder.h / encoder.c        # 正交编码器
  ir_board.h / ir_board.c      # 红外灯板 / 灰度阵列
  imu_gyro.h / imu_gyro.c      # 陀螺/IMU（见 IMU_INS_PLAYBOOK）
  adc_sensor.h / adc_sensor.c  # 通用 ADC（电感/电池）
  key_beep.h / key_beep.c      # 按键蜂鸣
```

---

## M1 · TOF 测距（VL53 类 I2C，可改 UART 模块）

### `bsp/tof_vl53.h`

```c
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  float dist_mm;       // 有效距离
  uint8_t status;      // 0=ok, 1=short, 2=timeout, 3=invalid
  uint32_t stamp_ms;
  bool valid;
} tof_sample_t;

int  tof_init(uint8_t i2c_addr);          // 失败返回 <0
void tof_poll(void);                      // 非阻塞：启动/取回一轮
bool tof_get(tof_sample_t *out);          // 取最近有效值
bool tof_fresh(uint32_t now_ms, uint32_t max_age_ms);
float tof_dist_filtered_mm(void);         // 一阶低通后距离
```

### `bsp/tof_vl53.c`（核心逻辑，HAL 需对接）

```c
#include "tof_vl53.h"

/* === 板级对接：按工程改这 4 个 === */
extern int  hal_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *d, int n);
extern int  hal_i2c_read (uint8_t addr, uint8_t reg, uint8_t *d, int n);
extern uint32_t millis(void);
extern void delay_ms(uint32_t ms);

static uint8_t g_addr = 0x29;
static tof_sample_t g_raw;
static float g_filt = 0.f;
static uint8_t g_phase = 0; /* 0 idle 1 waiting */

#ifndef TOF_LP_ALPHA
#define TOF_LP_ALPHA 0.25f
#endif
#ifndef TOF_MIN_MM
#define TOF_MIN_MM 20.f
#endif
#ifndef TOF_MAX_MM
#define TOF_MAX_MM 2000.f
#endif

static float clampf(float x, float a, float b) {
  if (x < a) return a; if (x > b) return b; return x;
}

int tof_init(uint8_t i2c_addr) {
  g_addr = i2c_addr ? i2c_addr : 0x29;
  g_raw.valid = false;
  g_phase = 0;
  /* 示例：软复位/加载配置 —— 换成你的 VL53 API */
  uint8_t soft_reset = 0x00;
  if (hal_i2c_write(g_addr, 0x00, &soft_reset, 1) < 0) return -1;
  delay_ms(2);
  return 0;
}

void tof_poll(void) {
  uint32_t now = millis();
  if (g_phase == 0) {
    uint8_t start = 0x01; /* 启动单次测量，寄存器按手册改 */
    if (hal_i2c_write(g_addr, 0x00, &start, 1) == 0) g_phase = 1;
    return;
  }
  uint8_t ready = 0;
  if (hal_i2c_read(g_addr, 0x13, &ready, 1) < 0) { g_phase = 0; return; }
  if (!(ready & 0x01)) return; /* 未完成 */

  uint8_t buf[2] = {0};
  if (hal_i2c_read(g_addr, 0x1E, buf, 2) < 0) { g_phase = 0; return; }
  float mm = (float)((buf[0] << 8) | buf[1]); /* 大端示例 */

  g_raw.stamp_ms = now;
  if (mm < TOF_MIN_MM) { g_raw.status = 1; g_raw.valid = false; }
  else if (mm > TOF_MAX_MM) { g_raw.status = 2; g_raw.valid = false; }
  else {
    g_raw.dist_mm = mm;
    g_raw.status = 0;
    g_raw.valid = true;
    g_filt = (g_filt == 0.f) ? mm : (TOF_LP_ALPHA * mm + (1.f - TOF_LP_ALPHA) * g_filt);
  }
  g_phase = 0;
}

bool tof_get(tof_sample_t *out) {
  if (!out) return false;
  *out = g_raw;
  return g_raw.valid;
}

bool tof_fresh(uint32_t now_ms, uint32_t max_age_ms) {
  return g_raw.valid && (now_ms - g_raw.stamp_ms) <= max_age_ms;
}

float tof_dist_filtered_mm(void) { return g_filt; }
```

### 控制环用法

```c
tof_poll();
if (!tof_fresh(now, 50)) { /* 超时：减速或忽略避障 */ }
else if (tof_dist_filtered_mm() < 150.f) speed_limit = fminf(speed_limit, 0.3f);
```

测试：遮挡变近；拔线超时；强光/超量程 status≠0。

---

## M2 · 编码器（正交 / 脉冲方向）

### `bsp/encoder.h`

```c
#pragma once
#include <stdint.h>

typedef struct {
  int32_t count;          // 累计脉冲（可负）
  int32_t delta;          // 本周期增量
  float rpm;              // 转/分（可选）
  float speed_mms;        // 线速度 mm/s（需轮径）
  uint32_t stamp_ms;
} encoder_sample_t;

void encoder_init(uint8_t ch);                 // ch: 左0/右1…
void encoder_on_edge(uint8_t ch, int8_t dir);  // 中断里 +1/-1
void encoder_update(uint8_t ch, uint32_t now_ms, float dt_s);
encoder_sample_t encoder_get(uint8_t ch);
void encoder_reset(uint8_t ch);
```

### `bsp/encoder.c`

```c
#include "encoder.h"

#ifndef ENC_PPR
#define ENC_PPR 1024.f          /* 每转脉冲（含倍频后） */
#endif
#ifndef WHEEL_DIAM_MM
#define WHEEL_DIAM_MM 65.f
#endif

#define ENC_CH_MAX 4
static volatile int32_t g_cnt[ENC_CH_MAX];
static encoder_sample_t g_s[ENC_CH_MAX];

void encoder_init(uint8_t ch) {
  if (ch >= ENC_CH_MAX) return;
  g_cnt[ch] = 0;
  g_s[ch].count = g_s[ch].delta = 0;
  g_s[ch].rpm = g_s[ch].speed_mms = 0.f;
}

void encoder_on_edge(uint8_t ch, int8_t dir) {
  if (ch >= ENC_CH_MAX) return;
  g_cnt[ch] += (dir >= 0) ? 1 : -1;
}

void encoder_reset(uint8_t ch) {
  if (ch >= ENC_CH_MAX) return;
  g_cnt[ch] = 0;
  g_s[ch].count = 0;
}

void encoder_update(uint8_t ch, uint32_t now_ms, float dt_s) {
  if (ch >= ENC_CH_MAX || dt_s <= 1e-6f) return;
  int32_t c = g_cnt[ch];
  int32_t d = c - g_s[ch].count;
  g_s[ch].delta = d;
  g_s[ch].count = c;
  g_s[ch].stamp_ms = now_ms;
  float rev_s = ((float)d / ENC_PPR) / dt_s;
  g_s[ch].rpm = rev_s * 60.f;
  float circ = 3.1415926f * WHEEL_DIAM_MM;
  g_s[ch].speed_mms = rev_s * circ;
}

encoder_sample_t encoder_get(uint8_t ch) {
  encoder_sample_t z = {0};
  return (ch < ENC_CH_MAX) ? g_s[ch] : z;
}
```

### 速度环对接（P01）

```c
encoder_update(0, now, 0.005f);
encoder_update(1, now, 0.005f);
float v_meas = 0.5f * (encoder_get(0).speed_mms + encoder_get(1).speed_mms);
float e = v_ref - v_meas;
/* 增量 PI → PWM，记得限幅 */
```

测试：正转 count↑；反转↓；已知转速下 rpm 误差；PIT 周期稳定。

---

## M3 · 红外灯板 / 灰度巡线阵列

### `bsp/ir_board.h`

```c
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifndef IR_CH
#define IR_CH 8
#endif

typedef struct {
  uint16_t raw[IR_CH];     // ADC 原始
  uint8_t  bin[IR_CH];     // 0/1 黑线
  float    err;            // 归一化中线偏差 [-1,1]
  uint8_t  on_line;        // 在线上通道数
  bool     lost;           // 丢线
  uint32_t stamp_ms;
} ir_board_t;

void ir_board_init(void);
void ir_board_calib_white(void);  // 放白底采样
void ir_board_calib_black(void);  // 放黑线采样
void ir_board_update(uint32_t now_ms);
const ir_board_t *ir_board_get(void);
```

### `bsp/ir_board.c`

```c
#include "ir_board.h"

extern uint16_t hal_adc_read_ir(uint8_t ch);
extern uint32_t millis(void);

static ir_board_t g;
static uint16_t thr[IR_CH];
static uint16_t white_ref[IR_CH], black_ref[IR_CH];

/* 通道几何权重：左负右正，按灯板改 */
static const float k_pos[IR_CH] = {
  -1.00f, -0.70f, -0.40f, -0.15f, 0.15f, 0.40f, 0.70f, 1.00f
};

void ir_board_init(void) {
  for (int i = 0; i < IR_CH; ++i) {
    thr[i] = 2048;
    white_ref[i] = 3500;
    black_ref[i] = 800;
  }
}

void ir_board_calib_white(void) {
  for (int i = 0; i < IR_CH; ++i) white_ref[i] = hal_adc_read_ir(i);
  for (int i = 0; i < IR_CH; ++i)
    thr[i] = (uint16_t)((white_ref[i] + black_ref[i]) / 2);
}

void ir_board_calib_black(void) {
  for (int i = 0; i < IR_CH; ++i) black_ref[i] = hal_adc_read_ir(i);
  for (int i = 0; i < IR_CH; ++i)
    thr[i] = (uint16_t)((white_ref[i] + black_ref[i]) / 2);
}

void ir_board_update(uint32_t now_ms) {
  float sum_w = 0.f, sum_p = 0.f;
  uint8_t on = 0;
  for (int i = 0; i < IR_CH; ++i) {
    g.raw[i] = hal_adc_read_ir(i);
    /* 黑线更暗：raw < thr → 1；若硬件相反请取反 */
    g.bin[i] = (g.raw[i] < thr[i]) ? 1 : 0;
    if (g.bin[i]) {
      on++;
      sum_w += 1.f;
      sum_p += k_pos[i];
    }
  }
  g.on_line = on;
  g.lost = (on == 0);
  g.err = (sum_w > 0.f) ? (sum_p / sum_w) : g.err; /* 丢线保持上次 */
  g.stamp_ms = now_ms;
}

const ir_board_t *ir_board_get(void) { return &g; }
```

### 循迹对接（V03 / C02）

```c
ir_board_update(now);
const ir_board_t *ir = ir_board_get();
if (ir->lost) { /* 丢线策略：降速/上次方向/元素态 */ }
float u = Kp * ir->err + Kd * (ir->err - err_prev);
```

测试：白底全 0；压线 on_line≥1；左右偏 err 符号正确；标定后阈值稳。

---

## M4 · 电感 ADC（疯狂电路等，简版）

```c
/* bsp/adc_sensor.c 片段 */
#define IND_N 6
static float ind[IND_N];
void inductance_update(void) {
  for (int i = 0; i < IND_N; ++i)
    ind[i] = 0.2f * hal_adc_read_ind(i) + 0.8f * ind[i]; /* 低通 */
}
/* 归一化后算中线，同 ir_board 加权思路 */
```

---

## M5 · 按键 + 蜂鸣（调试/发车）

```c
typedef enum { KEY_NONE=0, KEY_SHORT, KEY_LONG } key_evt_t;
key_evt_t key_scan(uint32_t now_ms); /* 消抖 20ms；长按 >800ms */
void beep_ms(uint16_t ms);
```

---

## 主循环组装示例

```c
void app_1ms(void) { /* 编码器中断外的结算可 5ms */ }

void app_5ms(void) {
  uint32_t now = millis();
  encoder_update(0, now, 0.005f);
  encoder_update(1, now, 0.005f);
  ir_board_update(now);
  tof_poll();
  /* 控制：err=ir->err；避障=tof；速度=encoder */
}
```

## 赛题映射速查

| 赛题需求 | 用模块 |
|----------|--------|
| 巡线 / 灯板 | M3 ir_board |
| 测速 / 里程 / 闭环 | M2 encoder |
| 避障 / 跟停 / 入库距 | M1 TOF |
| 电磁组 | M4 inductance |
| 发车 / 模式 | M5 key_beep |
| 平衡 / 惯导 | `IMU_INS_PLAYBOOK.md`（可另建） |

写整车代码时：先把本文件模块放进 `bsp/`，再在 `perception/` / `control/` 调用。
