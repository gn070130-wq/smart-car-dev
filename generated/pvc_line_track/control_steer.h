#ifndef CONTROL_STEER_H_
#define CONTROL_STEER_H_

typedef struct {
  float kp, kd;
  float umax;     /* 差速修正限幅 */
  float v_base;   /* 基础速度指令（PWM 或归一化） */
  float vmin, vmax;
  float err_prev;
} steer_pd_t;

void steer_pd_init(steer_pd_t *s, float kp, float kd, float v_base);

/** err∈[-1,1] → 左右轮速度指令 */
void steer_pd_apply(steer_pd_t *s, float err, float speed_scale, int lost,
                    float *out_l, float *out_r);

#endif
