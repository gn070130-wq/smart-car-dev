#ifndef USER_SAFETY_H_
#define USER_SAFETY_H_

typedef struct {
  int estop;
  float max_tilt_deg;
} SafetyState;

void safety_init(void);
void safety_feed_tilt(float pitch_deg);
int safety_ok(void);
void safety_clamp_motor(float *l, float *r, float max_abs);

#endif
