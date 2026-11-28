#include "rknn_bridge.h"

static uint32_t last_infer_ms;
static float phase;

void rknn_bridge_init(void) {
  last_infer_ms = 0;
  phase = 0.f;
}

void rknn_bridge_poll(uint32_t now_ms, RknnResult *out) {
  /* 控制环每拍调用；推理仅 ~50ms 刷新，体现异步新鲜度 */
  if (now_ms - last_infer_ms >= 50u || last_infer_ms == 0) {
    last_infer_ms = now_ms;
    phase += 0.15f;
    if (phase > 1.f) phase = -1.f;
    out->err = phase;
    out->type_id = (phase > 0.f) ? 1 : 2;
    out->stamp_ms = now_ms;
  }
  out->fresh = (now_ms - out->stamp_ms) < 120u;
}
