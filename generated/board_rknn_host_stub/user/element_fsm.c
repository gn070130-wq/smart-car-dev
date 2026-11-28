#include "element_fsm.h"
#include <math.h>

static ElementState st;

void element_fsm_init(void) { st = EL_STRAIGHT; }

ElementOut element_fsm_step(float err, float sum_adc) {
  ElementOut o;
  float ae = err < 0 ? -err : err;
  if (sum_adc < 30.0f) st = EL_STOP;
  else if (ae > 0.55f) st = EL_CURVE;
  else st = EL_STRAIGHT;

  o.state = st;
  switch (st) {
    case EL_CURVE: o.v_ref = 0.45f; break;
    case EL_ROUNDABOUT: o.v_ref = 0.35f; break;
    case EL_STOP: o.v_ref = 0.0f; break;
    default: o.v_ref = 0.7f; break;
  }
  return o;
}
