#include "track_pvc.h"
#include <math.h>
#include <stdlib.h>

static float clampf(float x, float a, float b) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}

static inline uint8_t pix(const uint8_t *img, int x, int y) {
  if (x < 0) x = 0;
  if (x >= IMG_W) x = IMG_W - 1;
  if (y < 0) y = 0;
  if (y >= IMG_H) y = IMG_H - 1;
  return img[y * IMG_W + x];
}

static int is_line(uint8_t v, uint8_t th) {
#if PVC_DARK_LINE
  return v < th;
#else
  return v > th;
#endif
}

void track_pvc_init(track_pvc_t *t) {
  int i;
  for (i = 0; i < IMG_H; ++i) {
    t->left[i] = t->right[i] = t->mid[i] = (uint8_t)(IMG_W / 2);
    t->valid_l[i] = t->valid_r[i] = 0;
  }
  t->lost_l = t->lost_r = 0;
  t->err = t->err_prev = t->curv = 0.f;
  t->width_near = (float)IMG_W * 0.4f;
  t->th = 100;
  t->width_jump = 0;
  t->lost = false;
}

/* V01: PVC 抗反光——底部带宽均值估阈值，避开顶部强光 */
static uint8_t estimate_th(const uint8_t *img) {
  uint32_t sum = 0;
  int n = 0;
  int y, x;
  for (y = IMG_H - 10; y < IMG_H; ++y) {
    for (x = IMG_W / 6; x < IMG_W - IMG_W / 6; x += 3) {
      sum += pix(img, x, y);
      ++n;
    }
  }
  {
    uint8_t mean = (uint8_t)(sum / (n ? n : 1));
#if PVC_DARK_LINE
    /* 黑线：阈值取在均值偏下，减轻 PVC 反光斑当线 */
    return (uint8_t)(mean > 50 ? mean - 25 : mean / 2);
#else
    return (uint8_t)(mean < 200 ? mean + 25 : 230);
#endif
  }
}

static int find_edge_left(const uint8_t *img, int y, int start, uint8_t th) {
  int x;
  for (x = start; x > 2; --x) {
    if (is_line(pix(img, x, y), th) && !is_line(pix(img, x - 1, y), th))
      return x;
  }
  return -1;
}

static int find_edge_right(const uint8_t *img, int y, int start, uint8_t th) {
  int x;
  for (x = start; x < IMG_W - 3; ++x) {
    if (is_line(pix(img, x, y), th) && !is_line(pix(img, x + 1, y), th))
      return x;
  }
  return -1;
}

static void search_edges(const uint8_t *img, track_pvc_t *t) {
  int y;
  int last_l = IMG_W / 2;
  int last_r = IMG_W / 2;
  float prev_w;

  t->th = estimate_th(img);
  t->lost_l = t->lost_r = 0;

  for (y = IMG_H - 1; y >= 8; --y) {
    int seed = (last_l + last_r) / 2;
    int L = find_edge_left(img, y, seed, t->th);
    int R = find_edge_right(img, y, seed, t->th);

    if (L < 0) {
      t->valid_l[y] = 0;
      t->lost_l++;
      L = last_l;
    } else {
      /* 接缝毛刺：与上行限幅跳变 */
      if (t->valid_l[y + 1] && abs(L - last_l) > 18)
        L = (L + last_l) / 2;
      t->valid_l[y] = 1;
      last_l = L;
    }

    if (R < 0) {
      t->valid_r[y] = 0;
      t->lost_r++;
      R = last_r;
    } else {
      if (t->valid_r[y + 1] && abs(R - last_r) > 18)
        R = (R + last_r) / 2;
      t->valid_r[y] = 1;
      last_r = R;
    }

    if (L > R) {
      int tmp = L;
      L = R;
      R = tmp;
    }
    t->left[y] = (uint8_t)L;
    t->right[y] = (uint8_t)R;
    t->mid[y] = (uint8_t)((L + R) / 2);
  }

  for (y = 0; y < 8; ++y) {
    t->left[y] = t->left[8];
    t->right[y] = t->right[8];
    t->mid[y] = t->mid[8];
    t->valid_l[y] = t->valid_r[y] = 0;
  }

  prev_w = t->width_near;
  t->width_near = (float)(t->right[IMG_H - 12] - t->left[IMG_H - 12]);
  t->width_jump = (fabsf(t->width_near - prev_w) > 18.f) ? 1 : 0;
  t->lost = (t->lost_l > (IMG_H / 3) && t->lost_r > (IMG_H / 3));
}

void track_pvc_use_single_edge(track_pvc_t *t, int side_left, int half_width) {
  int y;
  for (y = 8; y < IMG_H; ++y) {
    int m;
    if (side_left)
      m = (int)t->left[y] + half_width;
    else
      m = (int)t->right[y] - half_width;
    if (m < 0) m = 0;
    if (m >= IMG_W) m = IMG_W - 1;
    t->mid[y] = (uint8_t)m;
  }
}

static void compute_err(track_pvc_t *t, int near_row, int far_row, float far_w) {
  float cx, e_near, e_far;
  if (near_row < 8) near_row = 8;
  if (far_row >= IMG_H) far_row = IMG_H - 1;
  if (far_row <= near_row) far_row = near_row + 1;

  cx = 0.5f * (float)(IMG_W - 1);
  e_near = ((float)t->mid[near_row] - cx) / cx;
  e_far = ((float)t->mid[far_row] - cx) / cx;
  far_w = clampf(far_w, 0.f, 1.f);

  t->err_prev = t->err;
  t->err = (1.f - far_w) * e_near + far_w * e_far;
  t->curv = e_far - e_near;

  if (t->lost) {
    /* 丢线保持符号，幅度衰减，避免 PVC 反光整幅丢线猛打舵 */
    t->err = 0.7f * t->err_prev;
  }
}

void track_pvc_process(const uint8_t *gray, track_pvc_t *t,
                       int near_row, int far_row, float far_w) {
  search_edges(gray, t);
  compute_err(t, near_row, far_row, far_w);
}
