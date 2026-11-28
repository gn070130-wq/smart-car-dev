#include "vision_ipc.h"

static vision_to_ctrl_t g_snap;

void vision_ipc_init(void) {
  g_snap.err = 0;
  g_snap.element = 0;
  g_snap.valid = 0;
  g_snap.stamp_ms = 0;
}

void vision_ipc_publish(float err, uint8_t element, uint32_t now_ms) {
  g_snap.err = err;
  g_snap.element = element;
  g_snap.valid = 1;
  g_snap.stamp_ms = now_ms;
}

int vision_ipc_fetch(vision_to_ctrl_t *out, uint32_t now_ms, uint32_t max_age_ms) {
  if (!g_snap.valid) return 0;
  if ((now_ms - g_snap.stamp_ms) > max_age_ms) {
    out->valid = 0;
    out->err = 0;
    return 0;
  }
  *out = g_snap;
  return 1;
}
