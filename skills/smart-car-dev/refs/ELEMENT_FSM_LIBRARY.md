# 元素状态机库（S01）

## 开源依据

| 来源 | 要点 |
|------|------|
| [0Leeeezy0/20th_smart_vision](https://github.com/0Leeeezy0/20th_smart_vision) `fsm.c` | common_path ↔ L/R_circle_in/out、斑马、推箱 |
| [a233a2/2022-17th-SmartCar-WirelessCharge](https://github.com/a233a2/2022-17th-SmartCar-WirelessCharge) | 换边线 + 陀螺累计角出元素（参考 AuTop） |
| [xiaomeiyan233/2025-Smart-car](https://github.com/xiaomeiyan233/2025-Smart-car) | 环岛/车库状态机 + 特殊点 + 陀螺角 |
| 本地 yunvfeng `ring.c` | FIRST→入环补线→环内→回正 |

## 设计原则（开源共识）

1. **进入 / 保持 / 退出** 条件写死，禁止非法跳转  
2. 环岛/三叉优先 **换边线** 或 **补一条控制边**，少改全局阈值  
3. 出环用 **陀螺积分角** 或 **里程**，比纯图像更稳  
4. 每态独立 **限速表**；超时必须退出到 `NORMAL` 或 `FAULT`

## 通用状态枚举

```c
typedef enum {
  EL_NORMAL = 0,
  EL_CROSS_PRE,
  EL_CROSS_PASS,
  EL_RING_L_IN, EL_RING_L, EL_RING_L_OUT,
  EL_RING_R_IN, EL_RING_R, EL_RING_R_OUT,
  EL_ZEBRA,
  EL_RAMP,
  EL_OBSTACLE_AVOID,
  EL_FAULT
} element_state_t;

typedef struct {
  element_state_t state;
  float yaw0;            /* 进入元素时航向 */
  float yaw_acc;         /* 累计转角 rad 或 deg，统一即可 */
  float dist_acc;        /* 里程累计 */
  uint32_t t0_ms;
  uint8_t  side;         /* 0左 1右 */
  float    speed_scale;  /* 0~1 限速系数 */
} element_fsm_t;
```

## 特征检测（图像侧钩子）

```c
typedef struct {
  int lost_l, lost_r;          /* 丢边行数 */
  int width_jump;              /* 赛道宽度突变 */
  int zebra_stripes;           /* 斑马条数 */
  int fork_like;               /* 三叉/分叉候选 */
  float track_width_near;      /* 近端宽度 */
} element_feat_t;

/* 伪代码：宽度突变 + 单侧丢边 → 环岛候选 */
int feat_ring_candidate(const element_feat_t *f, int *side) {
  if (f->lost_l > 8 && f->lost_r < 3 && f->width_jump) { *side = 0; return 1; }
  if (f->lost_r > 8 && f->lost_l < 3 && f->width_jump) { *side = 1; return 1; }
  return 0;
}
```

## 核心状态机

```c
#include <math.h>

#ifndef RING_IN_ANGLE
#define RING_IN_ANGLE   40.f   /* deg */
#endif
#ifndef RING_OUT_ANGLE
#define RING_OUT_ANGLE  280.f
#endif
#ifndef EL_TIMEOUT_MS
#define EL_TIMEOUT_MS   4000
#endif

static float ang_diff_deg(float a, float b) {
  float d = a - b;
  while (d > 180.f) d -= 360.f;
  while (d < -180.f) d += 360.f;
  return d;
}

void element_fsm_init(element_fsm_t *e) {
  e->state = EL_NORMAL;
  e->yaw_acc = e->dist_acc = 0.f;
  e->speed_scale = 1.f;
}

void element_fsm_update(element_fsm_t *e,
                        const element_feat_t *feat,
                        float yaw_deg,
                        float dist_delta,
                        uint32_t now_ms,
                        /* 输出给循迹 */
                        int *force_single_edge, /* -1无, 0左, 1右 */
                        int *half_width) {
  *force_single_edge = -1;
  *half_width = 40;
  e->dist_acc += dist_delta;

  /* 全局超时保护 */
  if (e->state != EL_NORMAL && (now_ms - e->t0_ms) > EL_TIMEOUT_MS) {
    e->state = EL_NORMAL;
    e->speed_scale = 1.f;
    return;
  }

  switch (e->state) {
  case EL_NORMAL: {
    int side = 0;
    if (feat_ring_candidate(feat, &side)) {
      e->state = side ? EL_RING_R_IN : EL_RING_L_IN;
      e->side = (uint8_t)side;
      e->yaw0 = yaw_deg;
      e->yaw_acc = 0.f;
      e->t0_ms = now_ms;
      e->speed_scale = 0.7f;
    } else if (feat->zebra_stripes >= 3) {
      e->state = EL_ZEBRA;
      e->t0_ms = now_ms;
      e->dist_acc = 0.f;
      e->speed_scale = 0.5f;
    } else if (feat->lost_l > 5 && feat->lost_r > 5) {
      e->state = EL_CROSS_PRE;
      e->t0_ms = now_ms;
      e->speed_scale = 0.6f;
    }
  } break;

  case EL_CROSS_PRE:
    /* 双侧短暂丢边：保持中线外推，出十字后恢复 */
    if (feat->lost_l < 2 && feat->lost_r < 2) {
      e->state = EL_NORMAL;
      e->speed_scale = 1.f;
    }
    break;

  case EL_RING_L_IN:
  case EL_RING_R_IN: {
    *force_single_edge = e->side; /* 沿内侧边 */
    e->yaw_acc = fabsf(ang_diff_deg(yaw_deg, e->yaw0));
    if (e->yaw_acc > RING_IN_ANGLE) {
      e->state = e->side ? EL_RING_R : EL_RING_L;
      e->yaw0 = yaw_deg;
      e->yaw_acc = 0.f;
    }
  } break;

  case EL_RING_L:
  case EL_RING_R: {
    *force_single_edge = e->side;
    e->yaw_acc += fabsf(ang_diff_deg(yaw_deg, e->yaw0));
    e->yaw0 = yaw_deg;
    if (e->yaw_acc > RING_OUT_ANGLE) {
      e->state = e->side ? EL_RING_R_OUT : EL_RING_L_OUT;
      e->t0_ms = now_ms;
      e->yaw_acc = 0.f;
      e->yaw0 = yaw_deg;
    }
  } break;

  case EL_RING_L_OUT:
  case EL_RING_R_OUT: {
    /* 出环：短暂补线/单边，然后回 NORMAL */
    *force_single_edge = e->side;
    e->yaw_acc = fabsf(ang_diff_deg(yaw_deg, e->yaw0));
    if (e->yaw_acc > 35.f || (feat->lost_l < 2 && feat->lost_r < 2)) {
      e->state = EL_NORMAL;
      e->speed_scale = 1.f;
    }
  } break;

  case EL_ZEBRA:
    if (e->dist_acc > 0.25f /* m，按实车改 */) {
      /* 可在此停车或忽略 */
      e->state = EL_NORMAL;
      e->speed_scale = 1.f;
    }
    break;

  case EL_OBSTACLE_AVOID:
    /* 与视觉避障模块配合：里程门控退出 */
    if (e->dist_acc > 0.4f) {
      e->state = EL_NORMAL;
      e->speed_scale = 1.f;
    }
    break;

  default:
    e->state = EL_NORMAL;
    break;
  }
}
```

## 与循迹对接

```c
track_process_frame(img, &tr, near_row, far_row, far_w);
int edge = -1, half = 40;
element_fsm_update(&el, &feat, yaw_deg, d_s, now, &edge, &half);
if (edge >= 0) track_use_single_edge(&tr, edge == 0, half);
track_compute_err(&tr, near_row, far_row, far_w);
v_cmd *= el.speed_scale;
```

## 测试清单

- 直道不误进环岛  
- 左/右环能进能出，超时能回 NORMAL  
- 十字短暂丢边不甩尾  
- 斑马只触发一次（加冷却距离）  
