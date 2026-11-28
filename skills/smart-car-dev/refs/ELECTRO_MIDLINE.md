# 电磁中线算法（疯狂电路 / 电磁循迹）

## 开源与资料依据

| 来源 | 要点 |
|------|------|
| 逐飞 [Electromagnetic_Induction_Module](https://gitee.com/seekfree/Electromagnetic_Induction_Module_Project) | 电磁模块硬件与采集 |
| [Blackerrr/16th-smart-car](https://github.com/Blackerrr/16th-smart-car) | TC264 电磁巡线 + 摄像头元素 |
| 竞赛常用写法 | 多电感归一化 → 差比和 / 加权位置 → err |
| 本地疯狂电路分析文档 | 加权偏差、远近融合思路可迁移到电感行 |

## 典型电感布局（示意）

```text
车头前进方向
  L2   L1   C   R1   R2     （水平一排，可加垂向电感）
位置权: -2   -1   0   +1   +2
```

## 全链路

```text
ADC原始 → 去零偏/低通 → 除以本通道最大值归一化
        → 差比和 或 加权质心 → err∈[-1,1]
        → 丢线/出界检测 → 转向PD + 速度门控
```

## 可粘贴实现

```c
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef IND_N
#define IND_N 5
#endif

typedef struct {
  float raw[IND_N];
  float norm[IND_N];
  float err;          /* [-1,1] 左负右正，可按车改 */
  float sum;          /* 能量：越小越可能丢线 */
  bool  lost;
  uint32_t stamp_ms;
} electro_t;

/* 位置权重：按实车电感间距改 */
static const float k_pos[IND_N] = { -1.00f, -0.50f, 0.f, 0.50f, 1.00f };

static float clampf(float x, float a, float b) {
  if (x < a) return a; if (x > b) return b; return x;
}

void electro_init(electro_t *e) {
  for (int i = 0; i < IND_N; ++i) e->raw[i] = e->norm[i] = 0.f;
  e->err = 0.f; e->sum = 0.f; e->lost = true;
}

/* 一阶低通采集；hal_adc_ind(i) 返回原始 ADC */
void electro_sample(electro_t *e, float (*hal_adc_ind)(int), float alpha) {
  for (int i = 0; i < IND_N; ++i) {
    float v = (float)hal_adc_ind(i);
    e->raw[i] = alpha * v + (1.f - alpha) * e->raw[i];
  }
}

/*
 * 动态归一化：除以本帧最大通道，抑制电池/高度变化。
 * 也可改成“除以开机白电平标定值”。
 */
void electro_normalize(electro_t *e) {
  float mx = 1.f;
  for (int i = 0; i < IND_N; ++i) if (e->raw[i] > mx) mx = e->raw[i];
  for (int i = 0; i < IND_N; ++i) e->norm[i] = e->raw[i] / mx;
}

/* 方法A：差比和（经典双电感也可退化成 (R-L)/(R+L)） */
float electro_err_diff_ratio(const electro_t *e) {
  /* 示例：用最左最右 */
  float L = e->norm[0];
  float R = e->norm[IND_N - 1];
  float s = L + R;
  if (s < 1e-3f) return 0.f;
  return clampf((R - L) / s, -1.f, 1.f);
}

/* 方法B：加权质心（多电感推荐） */
float electro_err_centroid(const electro_t *e) {
  float sw = 0.f, sp = 0.f;
  for (int i = 0; i < IND_N; ++i) {
    float w = e->norm[i];
    /* 可选：抑制噪声地板 */
    if (w < 0.05f) continue;
    sw += w;
    sp += w * k_pos[i];
  }
  if (sw < 1e-3f) return 0.f;
  return clampf(sp / sw, -1.f, 1.f);
}

/* 方法C：三电感水平 + 垂直前瞻（进弯提前） */
float electro_err_fuse(const electro_t *e_h, const electro_t *e_v, float k_v) {
  float eh = electro_err_centroid(e_h);
  float ev = electro_err_centroid(e_v);
  return clampf((1.f - k_v) * eh + k_v * ev, -1.f, 1.f);
}

void electro_update(electro_t *e, float (*hal_adc_ind)(int), uint32_t now_ms) {
  electro_sample(e, hal_adc_ind, 0.3f);
  electro_normalize(e);
  e->sum = 0.f;
  for (int i = 0; i < IND_N; ++i) e->sum += e->norm[i];
  e->lost = (e->sum < 0.35f);          /* 阈值实车标定 */
  e->err = electro_err_centroid(e);    /* 或 diff_ratio */
  if (e->lost) { /* 保持上次 err，由上层降速 */ }
  e->stamp_ms = now_ms;
}
```

## 接到转向（C02）

```c
electro_update(&el, hal_adc_ind, now);
float u = Kp * el.err + Kd * (el.err - err_prev);
err_prev = el.err;
if (el.lost) v_base *= 0.4f;
pwm_l = clampf(v_base - u, VMIN, VMAX);
pwm_r = clampf(v_base + u, VMIN, VMAX);
```

## 标定步骤

1. 车在导线正上方：各通道接近，err≈0  
2. 左偏/右偏：err 符号与舵机方向一致（不对就取反）  
3. 抬高车身：看归一化后 err 是否仍可用  
4. 丢线：离开导线 sum 下降，触发降速/回环  

## 测试

- 静止左右摆车，err 单调  
- 直道高速不抖（增大滤波或减小 Kd）  
- 出界 lost=1 且不积分饱和  
