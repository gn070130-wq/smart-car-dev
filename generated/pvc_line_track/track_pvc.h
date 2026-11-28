/**
 * PVC 赛道摄像头循迹（V01→V02→V03）
 * 适配反光/接缝：底部带阈值 + 边线低通 + 远近前瞻加权
 */
#ifndef TRACK_PVC_H_
#define TRACK_PVC_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef IMG_W
#define IMG_W 188
#endif
#ifndef IMG_H
#define IMG_H 120
#endif

/* 1=黑线（暗于阈值）；0=白线（亮于阈值）—— PVC 蓝底黑线常用 1 */
#ifndef PVC_DARK_LINE
#define PVC_DARK_LINE 1
#endif

typedef struct {
  uint8_t left[IMG_H];
  uint8_t right[IMG_H];
  uint8_t mid[IMG_H];
  uint8_t valid_l[IMG_H];
  uint8_t valid_r[IMG_H];
  uint16_t lost_l;
  uint16_t lost_r;
  float err;       /* [-1,1] 左负右正（可按车取反） */
  float err_prev;
  float curv;      /* 远-近，弯道代理 */
  float width_near;
  uint8_t th;
  uint8_t width_jump; /* 近端宽度突变标志 */
  bool lost;
} track_pvc_t;

void track_pvc_init(track_pvc_t *t);

/* 一帧：搜边 →（可选单边）→ 算 err */
void track_pvc_process(const uint8_t *gray, track_pvc_t *t,
                       int near_row, int far_row, float far_w);

/* 环岛：0=沿左边+半宽，1=沿右边-半宽 */
void track_pvc_use_single_edge(track_pvc_t *t, int side_left, int half_width);

#endif /* TRACK_PVC_H_ */
