#include "control.h"
#include "element_fsm.h"
#include "hal.h"
#include "safety.h"
#include "track_line.h"

#define N_IND 6

void isr_tick(void) {
  float adc[N_IND];
  float ax, ay, az, gx, gy, gz;
  float err, sum;
  float l, r;
  ElementOut el;
  int i;

  hal_adc_inductance(adc, N_IND);
  sum = 0;
  for (i = 0; i < N_IND; ++i) sum += adc[i];
  err = track_midline_err(adc, N_IND);

  hal_imu_read(&ax, &ay, &az, &gx, &gy, &gz);
  safety_feed_tilt(ax * 57.3f); /* stub：用 ax 近似俯仰 */

  el = element_fsm_step(err, sum);
  control_step(err, el.v_ref, gz, &l, &r);
  safety_clamp_motor(&l, &r, 1.0f);
  if (!safety_ok()) {
    l = r = 0;
    hal_fan_set(0);
  }
  hal_motor_set(l, r);
}
