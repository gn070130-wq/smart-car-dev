#include "safety.h"

static SafetyState s;

void safety_init(void) {
  s.estop = 0;
  s.max_tilt_deg = 45.0f;
}

void safety_feed_tilt(float pitch_deg) {
  if (pitch_deg > s.max_tilt_deg || pitch_deg < -s.max_tilt_deg) s.estop = 1;
}

int safety_ok(void) { return !s.estop; }

void safety_clamp_motor(float *l, float *r, float max_abs) {
  if (!safety_ok()) {
    *l = 0;
    *r = 0;
    return;
  }
  if (*l > max_abs) *l = max_abs;
  if (*l < -max_abs) *l = -max_abs;
  if (*r > max_abs) *r = max_abs;
  if (*r < -max_abs) *r = -max_abs;
}
