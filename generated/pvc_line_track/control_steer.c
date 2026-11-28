#include "control_steer.h"

static float clampf(float x, float a, float b) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}

void steer_pd_init(steer_pd_t *s, float kp, float kd, float v_base) {
  s->kp = kp;
  s->kd = kd;
  s->umax = 0.55f * v_base;
  s->v_base = v_base;
  s->vmin = 0.f;
  s->vmax = v_base * 1.2f;
  s->err_prev = 0.f;
}

void steer_pd_apply(steer_pd_t *s, float err, float speed_scale, int lost,
                    float *out_l, float *out_r) {
  float de = err - s->err_prev;
  float u;
  float v;

  s->err_prev = err;
  u = s->kp * err + s->kd * de;
  u = clampf(u, -s->umax, s->umax);

  v = s->v_base * speed_scale;
  if (lost)
    v *= 0.4f;
  /* 弯道略减速：|err| 大时 */
  {
    float ae = err < 0.f ? -err : err;
    if (ae > 0.25f)
      v *= (1.f - 0.35f * ae);
  }

  *out_l = clampf(v - u, s->vmin, s->vmax);
  *out_r = clampf(v + u, s->vmin, s->vmax);
}
