#include "control.h"

static CtrlGain g;
static float integ;

void control_init(const CtrlGain *gain) {
  g = *gain;
  integ = 0.0f;
}

void control_step(float err, float v_ref, float gz, float *out_l, float *out_r) {
  float dir = g.kp_dir * err + g.kd_dir * (-gz * 0.01f);
  integ += g.ki_spd * (v_ref);
  if (integ > g.max_pwm) integ = g.max_pwm;
  if (integ < -g.max_pwm) integ = -g.max_pwm;
  {
    float base = g.kp_spd * v_ref + integ;
    float l = base - dir;
    float r = base + dir;
    if (l > g.max_pwm) l = g.max_pwm;
    if (l < -g.max_pwm) l = -g.max_pwm;
    if (r > g.max_pwm) r = g.max_pwm;
    if (r < -g.max_pwm) r = -g.max_pwm;
    *out_l = l;
    *out_r = r;
  }
}
