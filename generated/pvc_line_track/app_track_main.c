/**
 * PVC 赛道循迹主循环
 * 板级接口见 hal.h；PC 编译链接 hal_stub.c，上板换真实 HAL。
 */
#include "track_pvc.h"
#include "element_fsm.h"
#include "control_steer.h"
#include "hal.h"

#ifndef NEAR_ROW
#define NEAR_ROW 95
#endif
#ifndef FAR_ROW
#define FAR_ROW 55
#endif

static track_pvc_t g_track;
static element_fsm_t g_el;
static steer_pd_t g_steer;

void pvc_track_app_init(void) {
  track_pvc_init(&g_track);
  element_fsm_init(&g_el);
  steer_pd_init(&g_steer, 0.85f, 0.12f, 1800.f);
}

void pvc_track_app_once(void) {
  const uint8_t *img = hal_camera_gray();
  uint32_t now = hal_millis();
  float yaw = hal_yaw_deg();
  int force_edge = -1;
  int half_w = 40;
  float far_w;
  float vl, vr;

  if (!img)
    return;

  track_pvc_process(img, &g_track, NEAR_ROW, FAR_ROW, 0.35f);
  element_fsm_update(&g_el, &g_track, yaw, now, &force_edge, &half_w);

  if (force_edge >= 0) {
    track_pvc_use_single_edge(&g_track, force_edge == 0, half_w);
    far_w = 0.55f;
    {
      float cx = 0.5f * (float)(IMG_W - 1);
      float e_n = ((float)g_track.mid[NEAR_ROW] - cx) / cx;
      float e_f = ((float)g_track.mid[FAR_ROW] - cx) / cx;
      g_track.err = (1.f - far_w) * e_n + far_w * e_f;
    }
  } else if (g_track.curv > 0.15f || g_track.curv < -0.15f) {
    track_pvc_process(img, &g_track, NEAR_ROW, FAR_ROW, 0.55f);
  }

  steer_pd_apply(&g_steer, g_track.err, g_el.speed_scale, g_track.lost ? 1 : 0,
                 &vl, &vr);
  hal_motor_set(vl, vr);
}

#ifdef PVC_TRACK_HOST_DEMO
int main(void) {
  int i;
  pvc_track_app_init();
  /* 主机演示：跑有限帧后退出，便于 CI/本地验证 */
  for (i = 0; i < 200; ++i) {
    pvc_track_app_once();
    hal_delay_ms(5);
  }
  return 0;
}
#endif
