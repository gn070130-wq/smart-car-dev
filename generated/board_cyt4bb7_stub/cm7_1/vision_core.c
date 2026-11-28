#include "vision_ipc.h"
#include <math.h>
#include <stdint.h>

/* 模拟 CM7_1：根据时间产生中线误差，经 IPC 发布 */
void cm7_1_vision_step(uint32_t now_ms) {
  float t = (float)now_ms * 0.001f;
  float err = 0.35f * sinf(t * 2.0f);
  uint8_t el = (err > 0.4f || err < -0.4f) ? 1u : 0u;
  vision_ipc_publish(err, el, now_ms);
}
