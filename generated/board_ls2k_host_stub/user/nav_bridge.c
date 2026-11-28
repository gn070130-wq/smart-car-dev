#include "nav_bridge.h"

void nav_bridge_init(NavBridgeState *s) {
  s->goal_x = 1.0f;
  s->goal_y = 0.0f;
  s->stele_done = 0;
}

void nav_bridge_step(NavBridgeState *s, float *vx, float *w) {
  /* 简化：朝 goal 开环逼近，到点后计一次「识碑」 */
  if (s->goal_x > 0.05f) {
    *vx = 0.2f;
    *w = 0.0f;
    s->goal_x -= 0.05f;
  } else {
    *vx = 0.0f;
    *w = 0.0f;
    s->stele_done += 1;
    s->goal_x = 1.0f; /* 下一站 */
  }
}
