#ifndef NAV_BRIDGE_H_
#define NAV_BRIDGE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Linux 侧：OCR/规划结果 → 底盘 cmd（host 模拟） */
typedef struct {
  float goal_x;
  float goal_y;
  int stele_done; /* 碑文任务完成计数 */
} NavBridgeState;

void nav_bridge_init(NavBridgeState *s);
void nav_bridge_step(NavBridgeState *s, float *vx, float *w);

#ifdef __cplusplus
}
#endif

#endif
