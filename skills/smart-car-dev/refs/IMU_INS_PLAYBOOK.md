# 陀螺仪 / IMU / 惯导（P02 / P03）速查与代码

适用：单车定向、轮腿、卡丁姿态、飞檐倾角窗等。板级对接 `hal_imu_read`（见 `BOARD_TEMPLATES.md`）。

## 要点

1. 上电静止 **零偏标定**（陀螺平均；加速度可选水平检查）  
2. 姿态：互补滤波或 Mahony（加速度纠正倾角；无磁力计时 Yaw 仅积分→慢漂）  
3. 航位推算：`x += v*cos(yaw)*dt`（**禁止**加速度二次积分当主定位）  
4. 控制：角速度内环用陀螺；倾角外环用融合角；速度环尽量少和姿态强耦合  
5. 超时：`stamp` 过旧 → 降速/保持上次安全输出  

## 赛道用法

| 赛道 | 用法 |
|------|------|
| 单车 | Roll 串级平衡优先；Yaw+编码器 INS；轨迹回放 |
| 轮腿 | Pitch 平衡 1ms；速度作用腿零点；见 `CYT4BB7_DUAL_CORE.md` |
| 飞檐 | 环岛/跷跷板姿态窗；Z 轴陀螺前馈方向 |
| 卡丁 | 横摆/侧倾监测；CTE 为主，IMU 辅助 |

## 最小代码骨架

```c
typedef struct { float gx, gy, gz; float ax, ay, az; } imu_raw_t;
typedef struct {
  float pitch, roll, yaw;
  float bias_gx, bias_gy, bias_gz;
  uint32_t stamp_ms;
} imu_pose_t;

static imu_pose_t pose;
/* 由板级实现 */
extern imu_raw_t hal_imu_read(void);
extern void delay_ms(uint32_t ms);
extern uint32_t millis(void);

void imu_calib_gyro(int n) {
  double sx = 0, sy = 0, sz = 0;
  int i;
  for (i = 0; i < n; i++) {
    imu_raw_t r = hal_imu_read();
    sx += r.gx; sy += r.gy; sz += r.gz;
    delay_ms(1);
  }
  pose.bias_gx = (float)(sx / n);
  pose.bias_gy = (float)(sy / n);
  pose.bias_gz = (float)(sz / n);
}

void imu_update(float dt) {
  imu_raw_t r = hal_imu_read();
  float gx = r.gx - pose.bias_gx;
  float gy = r.gy - pose.bias_gy;
  float gz = r.gz - pose.bias_gz;
  float pitch_acc = atanf(r.ax / sqrtf(r.ay * r.ay + r.az * r.az + 1e-6f));
  float roll_acc = atanf(r.ay / (r.az == 0 ? 1e-6f : r.az));
  const float a = 0.98f;
  pose.pitch = a * (pose.pitch + gy * dt) + (1 - a) * pitch_acc;
  pose.roll = a * (pose.roll + gx * dt) + (1 - a) * roll_acc;
  pose.yaw += gz * dt;
  pose.stamp_ms = millis();
}

typedef struct { float x, y; } ins_t;
static ins_t ins;

void ins_step(float v_mms, float dt) {
  float v = v_mms * 0.001f;
  ins.x += v * cosf(pose.yaw) * dt;
  ins.y += v * sinf(pose.yaw) * dt;
}

int imu_fresh(uint32_t now_ms, uint32_t max_age_ms) {
  return (now_ms - pose.stamp_ms) <= max_age_ms;
}
```

## 串级平衡示意（单车/轮腿外环）

```c
/* angle_err = target_pitch - pose.pitch */
float balance_step(float angle_err, float gyro_y, float kp_a, float kd_a) {
  float u = kp_a * angle_err - kd_a * gyro_y;
  if (u > 1) u = 1;
  if (u < -1) u = -1;
  return u; /* → 电机/舵机，再经 safety 限幅 */
}
```

倾角超限（如 |pitch|>40°）必须 `estop`，无视寻迹。

## 测试

1. 静止标定后 gz≈0、pitch/roll 稳定  
2. 轻俯仰：pitch 跟随，松手回中  
3. 慢转：yaw 积分方向正确（会漂，属预期）  
4. 已知距离慢推：`ins.x` 量级接近（打滑则偏大）  

单车详解：`../danche-dingxiang/refs/ALGORITHMS.md`；轮腿：`CYT4BB7_DUAL_CORE.md`。
