#include "hal.h"
#include "nav_bridge.h"
#include "safety.h"
#include <stdio.h>

void isr_tick(void);

int main(void) {
  NavBridgeState nav;
  float vx = 0.f, w = 0.f;
  int steps;

  hal_init();
  safety_init();
  nav_bridge_init(&nav);

  printf("board_ls2k_host_stub (zouma / Linux bridge)\n");
  for (steps = 0; steps < 80; ++steps) {
    isr_tick();
    if (!safety_ok()) {
      hal_motor_set(0, 0);
      break;
    }
    nav_bridge_step(&nav, &vx, &w);
    hal_motor_set(vx - w, vx + w);
    if ((steps % 20) == 0) {
      printf("t=%u stele=%d vx=%.2f\n", (unsigned)hal_millis(), nav.stele_done, vx);
    }
    hal_delay_ms(5);
  }
  hal_motor_set(0, 0);
  printf("done steles=%d\n", nav.stele_done);
  return 0;
}
