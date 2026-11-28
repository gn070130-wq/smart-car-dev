#ifndef USER_CONTROL_H_
#define USER_CONTROL_H_

typedef struct {
  float kp_dir, kd_dir;
  float kp_spd, ki_spd;
  float max_pwm;
} CtrlGain;

void control_init(const CtrlGain *g);
/* err: 中线误差 -1..1；v_ref: 目标速度；gz: 陀螺 deg/s */
void control_step(float err, float v_ref, float gz, float *out_l, float *out_r);

#endif
