#include "hal.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static uint32_t g_t0;

static uint32_t now_ms(void) {
#ifdef _WIN32
  return (uint32_t)GetTickCount();
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
#endif
}

void hal_init(void) { g_t0 = now_ms(); }

uint32_t hal_millis(void) { return now_ms() - g_t0; }

void hal_delay_ms(uint32_t ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
  nanosleep(&ts, NULL);
#endif
}

void hal_adc_inductance(float *out, int n) {
  int i;
  float t = (float)hal_millis() * 0.001f;
  for (i = 0; i < n; ++i) {
    out[i] = 50.0f + 40.0f * sinf(t + (float)i * 0.7f);
  }
}

void hal_imu_read(float *ax, float *ay, float *az, float *gx, float *gy, float *gz) {
  (void)ax; (void)ay; (void)az; (void)gx; (void)gy;
  *gz = 0.0f;
  if (ax) *ax = 0;
  if (ay) *ay = 0;
  if (az) *az = 1.0f;
  if (gx) *gx = 0;
  if (gy) *gy = 0;
}

float hal_yaw_deg(void) { return 0.0f; }

void hal_encoder_read(int32_t *left, int32_t *right) {
  static int32_t L, R;
  L += 1;
  R += 1;
  if (left) *left = L;
  if (right) *right = R;
}

void hal_motor_set(float left, float right) {
  static int c;
  if ((++c % 50) == 0) {
    printf("[stub] motor L=%.2f R=%.2f\n", left, right);
  }
}

void hal_servo_set(float deg) { (void)deg; }
void hal_fan_set(float duty) { (void)duty; }
void hal_led(int on) { (void)on; }
int hal_key_pressed(void) { return 0; }
