#include "track_line.h"

/* 差比和示意：左右加权；实车按电感几何改权重 */
float track_midline_err(const float *adc, int n) {
  float sum = 0.0f, wsum = 0.0f;
  int i;
  if (n < 2) return 0.0f;
  for (i = 0; i < n; ++i) {
    float w = (float)i - 0.5f * (float)(n - 1);
    sum += adc[i];
    wsum += adc[i] * w;
  }
  if (sum < 1.0f) return 0.0f;
  {
    float e = wsum / sum;
    float norm = 0.5f * (float)(n - 1);
    if (norm < 1.0f) norm = 1.0f;
    e /= norm;
    if (e > 1.0f) e = 1.0f;
    if (e < -1.0f) e = -1.0f;
    return e;
  }
}
