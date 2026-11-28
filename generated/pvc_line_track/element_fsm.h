#ifndef ELEMENT_FSM_H_
#define ELEMENT_FSM_H_

#include <stdint.h>
#include "track_pvc.h"

typedef enum {
  EL_NORMAL = 0,
  EL_CROSS_PRE,
  EL_RING_L_IN,
  EL_RING_L,
  EL_RING_L_OUT,
  EL_RING_R_IN,
  EL_RING_R,
  EL_RING_R_OUT,
  EL_FAULT
} element_state_t;

typedef struct {
  element_state_t state;
  float yaw0;
  float yaw_acc;
  uint32_t t0_ms;
  uint8_t side; /* 0左 1右 */
  float speed_scale;
} element_fsm_t;

void element_fsm_init(element_fsm_t *e);

/**
 * 根据丢边/宽度突变 + 航向角更新状态。
 * force_edge: -1 无；0 沿左；1 沿右
 */
void element_fsm_update(element_fsm_t *e, const track_pvc_t *tr, float yaw_deg,
                        uint32_t now_ms, int *force_edge, int *half_width);

#endif
