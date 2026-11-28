#include "vision_ipc.h"
#include <stdio.h>

static float g_pitch;
static int g_estop;

void cm7_0_control_init(void) {
  g_pitch = 0;
  g_estop = 0;
}

/* 1ms 级控制：读视觉快照 + 简化平衡输出 */
void cm7_0_control_step(uint32_t now_ms, float pitch_deg, float *out_l, float *out_r) {
  vision_to_ctrl_t v;
  float bal, dir;

  g_pitch = pitch_deg;
  if (pitch_deg > 40.0f || pitch_deg < -40.0f) g_estop = 1;

  if (g_estop) {
    *out_l = *out_r = 0;
    return;
  }

  if (!vision_ipc_fetch(&v, now_ms, 80)) {
    v.err = 0;
    v.element = 0;
  }

  bal = -0.08f * pitch_deg; /* stub：俯仰→轮矩 */
  dir = 0.6f * v.err;
  if (v.element) {
    bal *= 0.6f;
  }
  *out_l = bal - dir;
  *out_r = bal + dir;
  if (*out_l > 1) *out_l = 1;
  if (*out_l < -1) *out_l = -1;
  if (*out_r > 1) *out_r = 1;
  if (*out_r < -1) *out_r = -1;
}

int cm7_0_estop(void) { return g_estop; }
