# Stanley / Pure Pursuit 完整代码（P05）

## 开源依据

| 来源 | 要点 |
|------|------|
| [AtsushiSakai/PythonRobotics](https://github.com/AtsushiSakai/PythonRobotics) Stanley / Pure Pursuit | 标准几何跟踪公式 |
| [Leo-7GOAT/path-tracking-controllers](https://github.com/Leo-7GOAT/path-tracking-controllers) | Stanley vs PP 对比与调参直觉 |
| 智能车单车定向 | 路径点列 + 平衡外再叠加弱转向 |

公式（自行车模型）：

- **Stanley**：`δ = θ_e + atan(k * e / v)`  
  - `θ_e`：航向误差；`e`：前轴横向误差（左正右负按坐标定义）  
- **Pure Pursuit**：`δ = atan(2 L sin(α) / Lf)`  
  - `α`：后轴指向目标点的夹角；`Lf = k_v * v + Lf0` 前视距  

下列为 **C 语言可粘贴** 精简实现（单位：m、rad）。

```c
#pragma once
#include <math.h>
#include <stdint.h>

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

typedef struct {
  float x, y, yaw;   /* 位姿 */
  float v;           /* 速度 m/s，|v|过小要保护 */
} vehicle_state_t;

typedef struct {
  float x[PATH_MAX];
  float y[PATH_MAX];
  float yaw[PATH_MAX]; /* 可选；Stanley 用 */
  int n;
} path_t;

static float clampf(float x, float a, float b) {
  if (x < a) return a; if (x > b) return b; return x;
}

static float wrap_pi(float a) {
  while (a > 3.14159265f) a -= 6.2831853f;
  while (a < -3.14159265f) a += 6.2831853f;
  return a;
}

static float dist2(float x0, float y0, float x1, float y1) {
  float dx = x0 - x1, dy = y0 - y1;
  return dx * dx + dy * dy;
}

/* 最近路径点（可从 last_idx 向前搜加速） */
int path_nearest(const path_t *p, float x, float y, int last_idx) {
  int i0 = last_idx < 0 ? 0 : last_idx;
  if (i0 >= p->n) i0 = p->n - 1;
  int best = i0;
  float bd = dist2(x, y, p->x[i0], p->y[i0]);
  for (int i = i0; i < p->n; ++i) {
    float d = dist2(x, y, p->x[i], p->y[i]);
    if (d < bd) { bd = d; best = i; }
    /* 已越过最近点且距离开始增大则停 */
    if (i > i0 + 2 && d > bd * 1.2f) break;
  }
  return best;
}

/* ========== Stanley ========== */
typedef struct {
  float k;          /* 横向增益，常见 0.5~5 */
  float k_soft;     /* 防低速除零，如 0.1 */
  float max_steer;  /* rad */
} stanley_cfg_t;

/*
 * 前轴位置近似：车长轴距 wheelbase，从前轴算横向误差。
 * e：路径左侧为正（右手系：误差 = 路径坐标系横向）。
 */
float stanley_steer(const vehicle_state_t *s, const path_t *path,
                    int *io_idx, float wheelbase,
                    const stanley_cfg_t *cfg) {
  int idx = path_nearest(path, s->x, s->y, *io_idx);
  *io_idx = idx;

  /* 前轴 */
  float fx = s->x + wheelbase * cosf(s->yaw);
  float fy = s->y + wheelbase * sinf(s->yaw);

  float px = path->x[idx], py = path->y[idx];
  float pyaw = path->yaw[idx];

  /* 横向误差：车辆前轴相对路径点，投影到路径法向 */
  float dx = fx - px, dy = fy - py;
  float e = cosf(pyaw + 1.5707963f) * dx + sinf(pyaw + 1.5707963f) * dy;
  /* 等价常用式：e = (fx-px)*cos(pyaw+pi/2)+(fy-py)*sin(...) 
     也可用叉积：e = (fy-py)*cos(pyaw) - (fx-px)*sin(pyaw) 视坐标定义取反 */

  float theta_e = wrap_pi(pyaw - s->yaw);
  float v = fabsf(s->v);
  float theta_d = atanf((cfg->k * e) / (cfg->k_soft + v));
  float delta = theta_e + theta_d;
  return clampf(delta, -cfg->max_steer, cfg->max_steer);
}

/* ========== Pure Pursuit ========== */
typedef struct {
  float kv;         /* Lf = kv*|v| + lf0 */
  float lf0;        /* 最小前视 m */
  float wheelbase;  /* L */
  float max_steer;
} pp_cfg_t;

int path_lookahead_index(const path_t *p, const vehicle_state_t *s,
                         int nearest, float Lf) {
  int ind = nearest;
  for (; ind < p->n - 1; ++ind) {
    float d = sqrtf(dist2(s->x, s->y, p->x[ind], p->y[ind]));
    if (d >= Lf) break;
  }
  return ind;
}

float pure_pursuit_steer(const vehicle_state_t *s, const path_t *path,
                         int *io_idx, const pp_cfg_t *cfg) {
  int nearest = path_nearest(path, s->x, s->y, *io_idx);
  float Lf = cfg->kv * fabsf(s->v) + cfg->lf0;
  int ind = path_lookahead_index(path, s, nearest, Lf);
  *io_idx = ind;

  float tx = path->x[ind], ty = path->y[ind];
  float alpha = wrap_pi(atan2f(ty - s->y, tx - s->x) - s->yaw);
  float delta = atanf(2.0f * cfg->wheelbase * sinf(alpha) / Lf);
  return clampf(delta, -cfg->max_steer, cfg->max_steer);
}
```

## 智能车用法建议

| 场景 | 推荐 |
|------|------|
| 单车绕锥 / 录路径回放 | Stanley 或 PP + 速度 PID |
| 只有中线 err、无全局路径 | 先用视觉/电磁 err + PD；有点列再用 PP |
| 低速大横向误差 | Stanley 增益勿过大，加 `k_soft` |
| 高速弯道 | PP 增大 `lf0`，避免切弯过度 |

## 与控制环对接

```c
/* path 由录径 P04 或规划得到；姿态来自 P02/P03 */
float delta = stanley_steer(&st, &path, &idx, 0.2f, &scfg);
/* 或 */ delta = pure_pursuit_steer(&st, &path, &idx, &pcfg);
servo_set_rad(delta);           /* 限幅已在算法内 */
speed_pid(v_ref, encoder_v);    /* 纵向 */
```

## 测试

1. 直线路径：横向误差 → 0，航向对齐  
2. 圆周：稳态误差有界，不振荡  
3. `v→0`：Stanley 不爆（靠 k_soft）  
4. 前视过小抖动 / 过大切弯：只调 `lf0/kv` 或 `k`  

Python 对照可读 PythonRobotics；嵌入式用本文件 C 版即可。
