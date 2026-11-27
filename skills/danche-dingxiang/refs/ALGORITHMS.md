# 算法拆解（单车定向）

每条格式：原理 · 输入/输出 · 场景 · 代码位置 · 关键参数。  
文末给出可直接改写的伪代码模板（用于「算法撰写」）。

---

## B1 IMU 零偏 + 四元数 EKF

- **原理**：静止采陀螺均值得零偏；EKF 用陀螺预测姿态，用加速度方向修正 Roll/Pitch；无磁力计时 Yaw 仅陀螺积分 → 慢漂移。  
- **输入**：`gyro_xyz`, `acc_xyz`；**输出**：`QEKF_INS.Roll/Pitch/Yaw`  
- **场景**：全科目底层姿态  
- **位置**：`IMU/Attitude.c` — `Attitude_Init`, `Attitude_Calculate`, `IMU_QuaternionEKF_Update`  
- **参数**：静止采样次数(~2000)、`gyroscope_threshold`、EKF 噪声阵、卡方检验门限  

## B2 航位推算 INS

- **原理**：`speed` 来自驱动板/编码器，不用加速度二次积分；`vx=v*cos(yaw)`, `vy=v*sin(yaw)`，`INS += v*dt`  
- **输入**：速度反馈、`Yaw`；**输出**：`INS_X`,`INS_Y`  
- **场景**：短时局部定位、回放寻迹坐标  
- **位置**：`INS_Speed_Calc()`（破晓：`dt=0.01`, `ENCODER_TIME=10ms`）  
- **参数**：速度换算系数（如 `/790*8.85*PI`）、打滑时误差放大  

## B3 轨迹采样 / Flash 回放

- **原理**：人工或低速沿目标路径按固定点距存 `yaw_id,X,Y`；比赛回放跟随  
- **输入**：里程触发；**输出**：路径数组 / Flash 页  
- **位置**：`Write_IMU`, `Write_IMU_XY`, `Flash_Init`, `Read_IMU_XY`  
- **参数**：`imu_point_step_cm`、Flash 页布局  

## B4 逐点导航

- **原理**：当前位置到目标点距离 + 方位角；航向误差经 `angle_limiter` → 转向环  
- **位置**：`data_analysis_ins`, `ServoGPS_Dispose`, `Car_control` 的 `keypoint`  
- **参数**：切点距离（报告例：科一~2m，科三~1m）  

## B5 IMU_Go 航向回放

- **原理**：按点推进，前瞻若干点的采样航向作目标，与当前 Yaw 差得 `delt_degree`  
- **优点**：简单好调；**缺点**：对初值/漂移敏感  
- **位置**：`IMU_Go()`  

## B6 Pure Pursuit + 曲率前馈

- **原理**：找最近点 → 前瞻点；横向误差 `atan(k*cte/v)`；曲率前馈 `atan(L*κ)`；合成目标航向  
- **位置**：`IMU_Pure_Pursuit_Control` / `IMU_Pure_Pursuit_Go`  
- **参数**：`pure_calc_offset`, `pure_k_lat`, `pure_correction_limit`, `pure_ff_gain`, `pure_wheelbase_m`, `pure_speed_base`  

## B7 Stanley

- **原理**：`δ = heading_error + atan(k*cte/speed)`  
- **位置**：`Stanley.c`；常作对比/备用  
- **场景**：贴线要求高、速度变化大的路径  

## B8 杜宾斯绕锥（杭电等）

- **原理**：已知锥点 + 最小转弯半径，生成 Dubins 控制点作轨迹  
- **场景**：科目锥桶；配合曲率/error 调速  

## B9 横滚平衡串级

- **原理**：角环 `balance_angle_PID` + 角速度环 `balance_gyro_PID`；转向误差进 `balance_turn_PID`；舵机改变横向动态维持不倒  
- **优先级**：**高于**速度与寻迹  
- **位置**：`balance.c`；舵机 PWM ~300Hz，中位约 90°/限幅 30–150°  
- **参数**：角环输出限幅（绕桶例 ±2000）、陀螺环 ±50、转向环输出 ±10、积分限幅  

## B10 增量式速度 PID

- **原理**：`Δu = Kp(e−e1)+Ki e+Kd(e−2e1+e2)`，累加限幅  
- **位置**：`balance_velocity_PID`；输出例 ±8000  
- **变体**：杭电半开环（PWM–转速表 + 反转急减速）  

## B11 UART FOC 帧

- **帧**：`0xA5 | 0x01 | dutyL_H L_L dutyR_H R_L | checksum`；速度回传功能字 `0x02`  
- **位置**：`small_driver_set_duty`, `uart_control_callback`  
- **风险**：丢帧 → 惯导与速度环一起坏  

## B12 余弦绕锥

- **原理**：锥间中心线改造成左右交替余弦偏移，`CONE_LOOK_AHEAD` / `CONE_CROSS_DISTANCE`  
- **位置**：`Task_Cone_Cos()`  

---

## 算法撰写模板（可直接落代码）

### 1) 航位推算（每 10ms）

```c
// yaw_rad: 当前航向(rad); speed: m/s; dt=0.01f
void INS_Update(float speed, float yaw_rad)
{
    float vx = speed * cosf(yaw_rad);
    float vy = speed * sinf(yaw_rad);
    INS_X += vx * 0.01f;
    INS_Y += vy * 0.01f;
}
```

### 2) 航向误差归一化

```c
float angle_limiter(float err_deg)
{
    while (err_deg > 180.f) err_deg -= 360.f;
    while (err_deg < -180.f) err_deg += 360.f;
    return err_deg;
}
```

### 3) 增量式速度 PID

```c
int16_t vel_pid_inc(pid_t *p, int16_t target, int16_t feedback)
{
    int16_t e = target - feedback;
    int32_t du = p->kp * (e - p->e1) + p->ki * e + p->kd * (e - 2*p->e1 + p->e2);
    p->e2 = p->e1; p->e1 = e;
    p->out += du;
    if (p->out > p->max) p->out = p->max;
    if (p->out < p->min) p->out = p->min;
    return (int16_t)p->out;
}
```

### 4) Pure Pursuit 横向修正（摘要）

```c
// lateral_m: 左正右负; v_base>0
float lat_corr = atanf(pure_k_lat * lateral_m / pure_speed_base);
if (lat_corr >  pure_correction_limit) lat_corr =  pure_correction_limit;
if (lat_corr < -pure_correction_limit) lat_corr = -pure_correction_limit;
float ff = atanf(pure_wheelbase_m * curvature) * pure_ff_gain;
float target_yaw = path_tangent + lat_corr + ff;
float err = angle_limiter(rad2deg(current_yaw - target_yaw));
```

### 5) 控制优先级（主循环心法）

```c
void Car_control(void)
{
    Attitude_Calculate();          // 先姿态
    if (fabsf(Roll) > ROLL_SAFE) { Protect_Stop(); return; }
    Update_Mission_State();        // 状态机
    Compute_Nav_Error();           // 寻迹误差
    Plan_Speed();                  // 目标速度
    balance_angle_PID();           // 平衡优先
    balance_gyro_PID();
    balance_turn_PID();            // 转向耦合
    balance_velocity_PID();
    Servo_Out(); UART_Motor_Out();
}
```

**撰写报告时建议结构**：问题→公式→参数表→伪代码→实车现象与误差源（漂移/打滑/延迟）。
