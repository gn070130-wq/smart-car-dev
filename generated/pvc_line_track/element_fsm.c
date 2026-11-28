#include "element_fsm.h"
#include <math.h>

#ifndef RING_IN_ANGLE
#define RING_IN_ANGLE 40.f
#endif
#ifndef RING_OUT_ANGLE
#define RING_OUT_ANGLE 280.f
#endif
#ifndef EL_TIMEOUT_MS
#define EL_TIMEOUT_MS 4000u
#endif

static float ang_diff_deg(float a, float b) {
  float d = a - b;
  while (d > 180.f)
    d -= 360.f;
  while (d < -180.f)
    d += 360.f;
  return d;
}

void element_fsm_init(element_fsm_t *e) {
  e->state = EL_NORMAL;
  e->yaw0 = e->yaw_acc = 0.f;
  e->t0_ms = 0;
  e->side = 0;
  e->speed_scale = 1.f;
}

static int ring_candidate(const track_pvc_t *tr, int *side) {
  /* PVC：单侧大量丢边 + 宽度跳变 → 环岛入口候选 */
  if (tr->lost_l > 8 && tr->lost_r < 3 && tr->width_jump) {
    *side = 0;
    return 1;
  }
  if (tr->lost_r > 8 && tr->lost_l < 3 && tr->width_jump) {
    *side = 1;
    return 1;
  }
  return 0;
}

void element_fsm_update(element_fsm_t *e, const track_pvc_t *tr, float yaw_deg,
                        uint32_t now_ms, int *force_edge, int *half_width) {
  int side = 0;
  *force_edge = -1;
  *half_width = 40;

  if (e->state != EL_NORMAL && (now_ms - e->t0_ms) > EL_TIMEOUT_MS) {
    e->state = EL_NORMAL;
    e->speed_scale = 1.f;
    return;
  }

  switch (e->state) {
  case EL_NORMAL:
    if (ring_candidate(tr, &side)) {
      e->state = side ? EL_RING_R_IN : EL_RING_L_IN;
      e->side = (uint8_t)side;
      e->yaw0 = yaw_deg;
      e->yaw_acc = 0.f;
      e->t0_ms = now_ms;
      e->speed_scale = 0.65f;
    } else if (tr->lost_l > 5 && tr->lost_r > 5) {
      e->state = EL_CROSS_PRE;
      e->t0_ms = now_ms;
      e->speed_scale = 0.55f;
    }
    break;

  case EL_CROSS_PRE:
    if (tr->lost_l < 2 && tr->lost_r < 2) {
      e->state = EL_NORMAL;
      e->speed_scale = 1.f;
    }
    break;

  case EL_RING_L_IN:
  case EL_RING_R_IN:
    *force_edge = e->side;
    e->yaw_acc = fabsf(ang_diff_deg(yaw_deg, e->yaw0));
    if (e->yaw_acc > RING_IN_ANGLE) {
      e->state = e->side ? EL_RING_R : EL_RING_L;
      e->yaw0 = yaw_deg;
      e->yaw_acc = 0.f;
    }
    break;

  case EL_RING_L:
  case EL_RING_R:
    *force_edge = e->side;
    e->yaw_acc += fabsf(ang_diff_deg(yaw_deg, e->yaw0));
    e->yaw0 = yaw_deg;
    if (e->yaw_acc > RING_OUT_ANGLE) {
      e->state = e->side ? EL_RING_R_OUT : EL_RING_L_OUT;
      e->t0_ms = now_ms;
      e->yaw_acc = 0.f;
      e->yaw0 = yaw_deg;
    }
    break;

  case EL_RING_L_OUT:
  case EL_RING_R_OUT:
    *force_edge = e->side;
    e->yaw_acc = fabsf(ang_diff_deg(yaw_deg, e->yaw0));
    if (e->yaw_acc > 35.f || (tr->lost_l < 2 && tr->lost_r < 2)) {
      e->state = EL_NORMAL;
      e->speed_scale = 1.f;
    }
    break;

  default:
    e->state = EL_NORMAL;
    e->speed_scale = 1.f;
    break;
  }
}
