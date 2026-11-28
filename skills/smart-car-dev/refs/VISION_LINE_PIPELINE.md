# 摄像头循迹全链路（V01→V02→V03）

## 开源依据（思路提炼，非逐字拷贝）

| 来源 | 要点 |
|------|------|
| [SJTU-AuTop/RT1064-Code](https://github.com/SJTU-AuTop/RT1064-Code) | 换边线处理元素，少硬补线 |
| [0Leeeezy0/20th_smart_vision](https://github.com/0Leeeezy0/20th_smart_vision) | 八邻域/最长白列/中线 + FSM |
| [xiaomeiyan233/2025-Smart-car](https://github.com/xiaomeiyan233/2025-Smart-car) | 逐行扫左右边、贝塞尔/加权中线、方差调速 |
| 本地 `yanguo-liuhen/.../yunvfeng/smart-car` | 参考列搜边、插值补行、Fitted_Midline、环岛补线 |
| 本地 `fengkuangdianluwansai` 文档 | 远近前瞻加权、高斯加权偏差 |

## 全链路数据流

```text
灰度图 → 阈值(固定/大津/底部统计)
      → 自下而上搜左右边线(断线插值/丢线标志)
      → 控制边线(元素态可换边)
      → 中线 mid[y]
      → 前瞻窗口加权 → err∈[-1,1]
      → 转向 PD / Stanley
```

## 可粘贴实现（通用 C，宽高可改）

```c
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef IMG_W
#define IMG_W 188
#endif
#ifndef IMG_H
#define IMG_H 120
#endif

typedef struct {
  uint8_t left[IMG_H];
  uint8_t right[IMG_H];
  uint8_t mid[IMG_H];
  uint8_t valid_l[IMG_H];
  uint8_t valid_r[IMG_H];
  uint8_t lost_l, lost_r;
  float err;          /* 归一化偏差 */
  float curv;         /* 简易曲率代理：远端-近端 */
  uint8_t th;
} track_frame_t;

static inline uint8_t pix(const uint8_t *img, int x, int y) {
  if (x < 0) x = 0; if (x >= IMG_W) x = IMG_W - 1;
  if (y < 0) y = 0; if (y >= IMG_H) y = IMG_H - 1;
  return img[y * IMG_W + x];
}

/* V01: 底部带宽估计阈值（黑线白底；若相反请反转比较） */
uint8_t track_estimate_th(const uint8_t *img) {
  uint32_t sum = 0; int n = 0;
  for (int y = IMG_H - 8; y < IMG_H; ++y)
    for (int x = 0; x < IMG_W; x += 4) { sum += pix(img, x, y); ++n; }
  uint8_t mean = (uint8_t)(sum / (n ? n : 1));
  return (uint8_t)(mean > 40 ? mean - 20 : mean / 2);
}

/* 从中心列向左右找边：黑线=暗于 th */
static int find_edge_left(const uint8_t *img, int y, int start, uint8_t th) {
  for (int x = start; x > 1; --x)
    if (pix(img, x, y) < th && pix(img, x - 1, y) >= th) return x;
  return -1;
}
static int find_edge_right(const uint8_t *img, int y, int start, uint8_t th) {
  for (int x = start; x < IMG_W - 2; ++x)
    if (pix(img, x, y) < th && pix(img, x + 1, y) >= th) return x;
  return -1;
}

/* V02: 自下而上搜边 + 断线插值 */
void track_search_edges(const uint8_t *img, track_frame_t *t) {
  t->th = track_estimate_th(img);
  int last_l = IMG_W / 2, last_r = IMG_W / 2;
  t->lost_l = t->lost_r = 0;
  for (int y = IMG_H - 1; y >= 8; --y) {
    int seed = (last_l + last_r) / 2;
    int L = find_edge_left(img, y, seed, t->th);
    int R = find_edge_right(img, y, seed, t->th);
    if (L < 0) { t->valid_l[y] = 0; t->lost_l++; L = last_l; }
    else { t->valid_l[y] = 1; last_l = L; }
    if (R < 0) { t->valid_r[y] = 0; t->lost_r++; R = last_r; }
    else { t->valid_r[y] = 1; last_r = R; }
    if (L > R) { int tmp = L; L = R; R = tmp; }
    t->left[y] = (uint8_t)L;
    t->right[y] = (uint8_t)R;
    t->mid[y] = (uint8_t)((L + R) / 2);
  }
  /* 近端未搜行填中心 */
  for (int y = 0; y < 8; ++y) {
    t->left[y] = t->left[8]; t->right[y] = t->right[8];
    t->mid[y] = t->mid[8]; t->valid_l[y] = t->valid_r[y] = 0;
  }
}

/* 单边循迹（环岛内侧）：只用一边 + 半宽估计中线 */
void track_use_single_edge(track_frame_t *t, int use_left, int half_width) {
  for (int y = 8; y < IMG_H; ++y) {
    if (use_left) t->mid[y] = (uint8_t)clampf(t->left[y] + half_width, 0, IMG_W - 1);
    else          t->mid[y] = (uint8_t)clampf(t->right[y] - half_width, 0, IMG_W - 1);
  }
}

#ifndef clampf
static float clampf(float x, float a, float b) {
  if (x < a) return a; if (x > b) return b; return x;
}
#endif

/* V03: 远近前瞻加权 → err */
void track_compute_err(track_frame_t *t, int near_row, int far_row, float far_w) {
  if (near_row < 0) near_row = 0;
  if (far_row >= IMG_H) far_row = IMG_H - 1;
  float cx = 0.5f * (IMG_W - 1);
  float e_near = (t->mid[near_row] - cx) / cx;
  float e_far  = (t->mid[far_row]  - cx) / cx;
  far_w = clampf(far_w, 0.f, 1.f);
  t->err = (1.f - far_w) * e_near + far_w * e_far;
  t->curv = e_far - e_near;
}

/* 一帧入口 */
void track_process_frame(const uint8_t *img, track_frame_t *t,
                         int near_row, int far_row, float far_w) {
  track_search_edges(img, t);
  track_compute_err(t, near_row, far_row, far_w);
}
```

## 调参与测试

| 项 | 建议 |
|----|------|
| 近前瞻 | 图像下部 70%–85% 行（稳车身） |
| 远前瞻 | 中上部；直角弯增大 `far_w` |
| 丢线 | `lost_l/r` 大时降速，保持上次 err 符号 |
| 测试 | 直道 err≈0；压左边 err&lt;0；拔相机超时保护 |

元素处理见 `ELEMENT_FSM_LIBRARY.md`（换边 / 补线 / 陀螺出环）。
