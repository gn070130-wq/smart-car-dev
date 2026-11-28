/**
 * PC 本地编译用 HAL 桩：无相机/电机时也能通过编译与链接。
 * 上板时删除本文件，改接逐飞/厂商库。
 */
#include "hal.h"
#include <string.h>

static uint8_t s_frame[IMG_W * IMG_H];
static uint32_t s_ms;

const uint8_t *hal_camera_gray(void) {
  /* 构造简单竖黑线，便于后续单测扩展 */
  memset(s_frame, 200, sizeof(s_frame));
  {
    int y;
    for (y = 0; y < IMG_H; ++y) {
      int x0 = IMG_W / 2 - 3;
      int x1 = IMG_W / 2 + 3;
      int x;
      for (x = x0; x <= x1; ++x)
        s_frame[y * IMG_W + x] = 30;
    }
  }
  return s_frame;
}

uint32_t hal_millis(void) {
  s_ms += 5;
  return s_ms;
}

float hal_yaw_deg(void) { return 0.f; }

void hal_motor_set(float left, float right) {
  (void)left;
  (void)right;
}

void hal_delay_ms(uint32_t ms) { (void)ms; }
