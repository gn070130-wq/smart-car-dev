#include "control.h"
#include "hal.h"
#include "rknn_bridge.h"
#include "safety.h"
#include <stdio.h>

void isr_tick(void);

int main(void) {
  CtrlGain g = {1.2f, 0.05f, 0.8f, 0.001f, 1.0f};
  RknnResult det = {0};
  int steps;

  hal_init();
  safety_init();
  control_init(&g);
  rknn_bridge_init();

  printf("board_rknn_host_stub (RK3588 async → MCU)\n");
  for (steps = 0; steps < 120; ++steps) {
    isr_tick();
    rknn_bridge_poll(hal_millis(), &det);
    if (!det.fresh || !safety_ok()) {
      hal_motor_set(0, 0);
    } else {
      float left = 0.4f - 0.3f * det.err;
      float right = 0.4f + 0.3f * det.err;
      hal_motor_set(left, right);
    }
    if ((steps % 30) == 0) {
      printf("t=%u type=%d err=%.2f fresh=%d\n",
             (unsigned)hal_millis(), det.type_id, det.err, det.fresh);
    }
    hal_delay_ms(5);
  }
  hal_motor_set(0, 0);
  printf("done\n");
  return 0;
}
