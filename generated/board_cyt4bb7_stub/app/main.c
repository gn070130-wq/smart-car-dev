#include "vision_ipc.h"
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void cm7_0_control_init(void);
void cm7_0_control_step(uint32_t now_ms, float pitch_deg, float *out_l, float *out_r);
int cm7_0_estop(void);
void cm7_1_vision_step(uint32_t now_ms);

static uint32_t now_ms(void) {
#ifdef _WIN32
  return (uint32_t)GetTickCount();
#else
  return 0;
#endif
}

static void delay_ms(uint32_t ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}

int main(void) {
  uint32_t t0, t;
  float l, r;
  int i;

  vision_ipc_init();
  cm7_0_control_init();
  t0 = now_ms();
  printf("board_cyt4bb7_stub dual-core host demo\n");

  for (i = 0; i < 200; ++i) {
    t = now_ms() - t0;
    /* CM7_1 稍慢：每 20ms 一帧 */
    if ((i % 4) == 0) cm7_1_vision_step(t);
    /* CM7_0 5ms */
    cm7_0_control_step(t, 2.0f, &l, &r);
    if ((i % 40) == 0) {
      printf("t=%u L=%.2f R=%.2f estop=%d\n", (unsigned)t, l, r, cm7_0_estop());
    }
    delay_ms(5);
  }
  printf("done\n");
  return 0;
}
