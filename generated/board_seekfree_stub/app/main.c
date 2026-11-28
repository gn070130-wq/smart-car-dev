#include "control.h"
#include "element_fsm.h"
#include "hal.h"
#include "safety.h"
#include <stdio.h>

void isr_tick(void);

int main(void) {
  CtrlGain g = {1.2f, 0.05f, 0.8f, 0.001f, 1.0f};
  int steps;

  hal_init();
  safety_init();
  element_fsm_init();
  control_init(&g);

  printf("board_seekfree_stub host demo\n");
  for (steps = 0; steps < 200; ++steps) {
    isr_tick();
    if ((steps % 40) == 0) {
      printf("t=%u ms ok=%d\n", (unsigned)hal_millis(), safety_ok());
    }
    hal_delay_ms(5);
  }
  hal_motor_set(0, 0);
  printf("done\n");
  return 0;
}
