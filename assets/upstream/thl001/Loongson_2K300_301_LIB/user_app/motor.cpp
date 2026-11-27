#include "motor.hpp"
#include <memory>

namespace
{
// ========== 硬件引脚配置 ==========
// 左轮PWM引脚
constexpr atim_pwm_pin_t kLeftMotorPwmPin  = ATIM_PWM0_PIN81;
// 右轮PWM引脚
constexpr atim_pwm_pin_t kRightMotorPwmPin = ATIM_PWM1_PIN82;
// 左轮方向控制GPIO引脚
constexpr gpio_pin_t kLeftMotorDirPin  = PIN_21;
// 右轮方向控制GPIO引脚
constexpr gpio_pin_t kRightMotorDirPin = PIN_22;
// 左轮编码器A相引脚
constexpr ls_enc_pwm_pin_t kLeftEncoderPin = ENC_PWM0_PIN64;
// 右轮编码器A相引脚
constexpr ls_enc_pwm_pin_t kRightEncoderPin = ENC_PWM1_PIN65;
// 左轮编码器方向/B相引脚
constexpr gpio_pin_t kLeftEncoderDirPin  = PIN_72;
// 右轮编码器方向/B相引脚
constexpr gpio_pin_t kRightEncoderDirPin = PIN_73;

// ========== 电机参数配置 ==========
// PWM频率：10kHz（越高越平滑，但CPU负担越大）
constexpr uint32_t kMotorPwmFreqHz = 10000;
// 默认初始化占空比：1000
constexpr int kDefaultInitDuty = 1000;
// 默认巡航速度：12（编码器计数/周期）
constexpr int kDefaultCruiseSpeed =12;
// 启动补偿PWM值：1200（电机静止时强制输出，帮助启动）
constexpr int kStartupCompensationPwm = 1200;
// 积分分离阈值：15（误差超过此值时禁用积分，防止积分饱和）
constexpr int kSpeedIntegralSeparation = 15;
// PID反转误差阈值：20（误差从正变负或负变正时，说明电机可能反转，需要衰减输出）
constexpr int kPidReverseErrorThreshold = 20;

// ========== 方向定义 ==========
// 小车前进时，左轮方向GPIO应输出的电平（false=低电平）
constexpr bool kLeftForwardDir  = false;   
// 小车前进时，右轮方向GPIO应输出的电平（true=高电平）
constexpr bool kRightForwardDir = true;  

// ========== 硬件对象声明 ==========
// 左轮PWM控制器
std::unique_ptr<ls_atim_pwm> left_motor_pwm;
// 右轮PWM控制器
std::unique_ptr<ls_atim_pwm> right_motor_pwm;
// 左轮方向GPIO
std::unique_ptr<ls_gpio> left_motor_dir;
// 右轮方向GPIO
std::unique_ptr<ls_gpio> right_motor_dir;
// 左轮编码器
std::unique_ptr<ls_encoder_pwm> left_motor_encoder;
// 右轮编码器
std::unique_ptr<ls_encoder_pwm> right_motor_encoder;

// 电机是否已初始化标志
bool motor_initialized = false;

// ========== 辅助函数 ==========
// PWM值限幅函数：确保PWM值在0到最大值之间
int clamp_pwm(int value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > ATIM_PWM_DUTY_MAX)
    {
        return ATIM_PWM_DUTY_MAX;
    }
    return value;
}

// PID输出限幅函数：确保PID输出在指定范围内
int clamp_pid_output(int value, int min_value, int max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

// 电机初始化检查函数：如果未初始化则自动初始化
void ensure_motor_ready()
{
    if (!motor_initialized)
    {
        Motor_Init1(kDefaultInitDuty);
    }
}
}

// ========== 全局变量（左轮） ==========
// 左轮编码器原始计数
int16_t encoder_Left = 0;
// 右轮编码器原始计数
int16_t encoder_Right = 0;
// 左轮编码器速度（经过滤波处理）
int16_t Speed_Encoder_l = 0;
// 左轮速度环P参数
float Speed_P_l = 0;
// 左轮速度环I参数
float Speed_I_l = 0;
// 左轮速度环D参数
float Speed_D_l = 0;
// 左轮速度误差（目标-实际）
int Speed_Erro_l = 0;
// 左轮目标速度
int Speed_Goal_l = 0;
// 左轮PID输出值
int Speed_PID_OUT_l = 0;
// 左轮上一次误差（用于P和D计算）
int Speed_Lasterro_l = 0;
// 左轮上上次误差（用于D计算）
int Speed_Preverro_l = 0;

// ========== 全局变量（右轮） ==========
// 右轮编码器速度
int16_t Speed_Encoder_r = 0;
// 右轮速度环P参数
float Speed_P_r = 0;
// 右轮速度环I参数
float Speed_I_r = 0;
// 右轮速度环D参数
float Speed_D_r = 0;
// 右轮速度误差
int Speed_Erro_r = 0;
// 右轮目标速度
int Speed_Goal_r = 0;
// 右轮PID输出值
int Speed_PID_OUT_r = 0;
// 右轮上一次误差
int Speed_Lasterro_r = 0;
// 右轮上上次误差
int Speed_Preverro_r = 0;

// ========== 全局参数 ==========
// PWM最大值：2500（正转最大占空比）
int PWM_Max = 2500;
// PWM最小值：-2500（反转最大占空比，实际输出时取绝对值）
int PWM_Min = -2500;
// 启动速度阈值：40（用于判断是否需要启动补偿）
int16_t Speed_Begin = 40;
// 期望速度（用于前馈补偿）
int16_t Speed_Expect = 0;
// 差速PID误差
float Diff_Speed_error = 0;
// 左轮差速目标速度（经过转向调整后的目标）
int16_t Diff_SpeedL_expect = 0;
// 右轮差速目标速度（经过转向调整后的目标）
int16_t Diff_SpeedR_expect = 0;
// 转向环P参数
float Diff_Kp = 3.5f;
// 转向环D参数
float Diff_Kd = 1.0f;
// 停止标志：1=停止，0=运行
uint8_t stop_flag = 0;

namespace
{
// 上一次转向误差（用于转向环D计算）
float last_turn_error = 0.0f;

// 重置速度控制状态：清零所有PID相关变量
void reset_speed_control_state()
{
    Speed_Encoder_l = 0;
    Speed_Encoder_r = 0;
    Speed_Erro_l = 0;
    Speed_Erro_r = 0;
    Speed_PID_OUT_l = 0;
    Speed_PID_OUT_r = 0;
    Speed_Lasterro_l = 0;
    Speed_Lasterro_r = 0;
    Speed_Preverro_l = 0;
    Speed_Preverro_r = 0;
    Diff_SpeedL_expect = 0;
    Diff_SpeedR_expect = 0;
    last_turn_error = 0.0f;
}

// 停止电机输出：设置目标速度为0，重置状态，输出0占空比
void stop_motor_output()
{
    Speed_Goal_l = 0;
    Speed_Goal_r = 0;
    reset_speed_control_state();
    Left_Motor_Pwm1(0, kLeftForwardDir);
    Right_Motor_Pwm1(0, kRightForwardDir);
}

// ========== 核心函数：速度环PID计算（增量式） ==========
// 参数：
//   expected_speed: 目标速度
//   measured_speed: 实际速度（编码器值）
//   p: P参数
//   i: I参数
//   d: D参数
//   last_error: 上一次误差（引用传递，用于保存状态）
//   prev_error: 上上次误差（引用传递，用于保存状态）
//   pid_output: PID输出（引用传递，用于保存累加值）
// 返回：当前误差
int update_speed_pid(int expected_speed,
                     int measured_speed,
                     float p,
                     float i,
                     float d,
                     int& last_error,
                     int& prev_error,
                     int& pid_output)
{
    // 1. 计算当前误差：目标速度 - 实际速度
    const int error = expected_speed - measured_speed;

    // 2. 积分分离机制：误差超过阈值时禁用积分，防止积分饱和
    float actual_i = i;
    if (my_abs(static_cast<float>(error)) > kSpeedIntegralSeparation)
    {
        actual_i = 0.0f;
    }

    // 3. 反转检测：误差符号突变说明电机可能反转，衰减输出防止冲击
    if ((last_error > kPidReverseErrorThreshold && error < -kPidReverseErrorThreshold) ||
        (last_error < -kPidReverseErrorThreshold && error > kPidReverseErrorThreshold))
    {
        pid_output = static_cast<int>(pid_output * 0.5f);
    }

    // 4. 增量式PID计算核心公式：
    // Δu = P*(e(k)-e(k-1)) + I*e(k) + D*(e(k)-2*e(k-1)+e(k-2))
    // pid_output += Δu（增量式：每次只加增量，不是重新计算）
    pid_output += static_cast<int>(p * (error - last_error) +
                                   actual_i * error +
                                   d * (error - 2 * last_error + prev_error));

    // 5. 启动补偿：目标速度>5且实际速度<5时，强制输出最小PWM值
    //    解决电机静止时摩擦力大、PID输出小无法启动的问题
    if (expected_speed > 5 && my_abs(static_cast<float>(measured_speed)) < 5)
    {
        if (pid_output < kStartupCompensationPwm)
        {
            pid_output = kStartupCompensationPwm;
        }
    }

    // 6. 反向抑制：目标速度为负（倒车）但输出为正时，衰减输出
    if (expected_speed < -5 && pid_output > 0)
    {
        pid_output = static_cast<int>(pid_output * 0.3f);
    }

    // 7. PID输出限幅：确保在PWM_Min到PWM_Max范围内
    pid_output = clamp_pid_output(pid_output, PWM_Min, PWM_Max);
    
    // 8. 更新误差历史：准备下次计算
    prev_error = last_error;
    last_error = error;
    
    // 9. 返回当前误差
    return error;
}
}

// ========== 编码器相关函数 ==========
// 获取左轮编码器值（取负值是因为安装方向相反）
float Encoder_Left1(void)
{
    ensure_motor_ready();
    encoder_Left = -static_cast<int16_t>(left_motor_encoder->encoder_get_count());
    return encoder_Left;
}

// 获取右轮编码器值
float Encoder_Right1(void)
{
    ensure_motor_ready();
    encoder_Right = static_cast<int16_t>(right_motor_encoder->encoder_get_count());
    return encoder_Right;
}

// 编码器测试函数：读取并清除编码器值
void Encoder_Test1(void)
{
    ensure_motor_ready();
    (void)Encoder_Left1();
    (void)Encoder_Right1();
}

// ========== 参数初始化函数 ==========
void Motor_Argument(void)
{
    // 设置默认巡航速度
    Speed_Goal_l = kDefaultCruiseSpeed;
    Speed_Goal_r = kDefaultCruiseSpeed;

    // 设置速度环PID参数（可调！）
    Speed_P_l = 8;       // 左轮P：影响响应速度，越大响应越快但可能振荡
    Speed_I_l = 2.5f;    // 左轮I：消除稳态误差，越大积分越强但可能超调
    Speed_D_l = 0.3f;    // 左轮D：抑制振荡，越大阻尼越强但可能迟钝

    // 右轮参数（通常与左轮对称）
    Speed_P_r = 8;
    Speed_I_r = 2.5f;
    Speed_D_r = 0.3f;
}

// ========== 电机初始化函数 ==========
void Motor_Init1(int duty)
{
    // 1. 限幅初始化占空比
    const uint32_t init_duty = static_cast<uint32_t>(clamp_pwm(duty));

    // 2. 创建硬件对象
    // PWM控制器：引脚、频率、初始占空比、极性反转
    left_motor_pwm  = std::make_unique<ls_atim_pwm>(kLeftMotorPwmPin,  kMotorPwmFreqHz, init_duty, ATIM_PWM_POL_INV);
    right_motor_pwm = std::make_unique<ls_atim_pwm>(kRightMotorPwmPin, kMotorPwmFreqHz, init_duty, ATIM_PWM_POL_INV);
    // 方向GPIO：引脚、输出模式
    left_motor_dir  = std::make_unique<ls_gpio>(kLeftMotorDirPin,  GPIO_MODE_OUT);
    right_motor_dir = std::make_unique<ls_gpio>(kRightMotorDirPin, GPIO_MODE_OUT);
    // 编码器：A相引脚、方向/B相引脚
    left_motor_encoder  = std::make_unique<ls_encoder_pwm>(kLeftEncoderPin,  kLeftEncoderDirPin);
    right_motor_encoder = std::make_unique<ls_encoder_pwm>(kRightEncoderPin, kRightEncoderDirPin);

    // 3. 设置初始方向为前进
    left_motor_dir->gpio_level_set(kLeftForwardDir  ? GPIO_HIGH : GPIO_LOW);
    right_motor_dir->gpio_level_set(kRightForwardDir ? GPIO_HIGH : GPIO_LOW);
    
    // 4. 设置初始PWM占空比为0（电机不转）
    left_motor_pwm->atim_pwm_set_duty(0);
    right_motor_pwm->atim_pwm_set_duty(0);

    // 5. 清零编码器值和PID状态
    encoder_Left = 0;
    encoder_Right = 0;
    reset_speed_control_state();
    
    // 6. 加载参数
    Motor_Argument();
    
    // 7. 标记初始化完成
    motor_initialized = true;
}

// ========== PWM输出函数 ==========
// 左轮PWM输出：duty=占空比，dir=方向
void Left_Motor_Pwm1(int duty, bool dir)
{
    ensure_motor_ready();
    // 设置方向GPIO电平
    left_motor_dir->gpio_level_set(dir ? GPIO_HIGH : GPIO_LOW);
    // 设置PWM占空比（限幅后转换为无符号整数）
    left_motor_pwm->atim_pwm_set_duty(static_cast<uint32_t>(clamp_pwm(duty)));
}

// 右轮PWM输出：duty=占空比，dir=方向
void Right_Motor_Pwm1(int duty, bool dir)
{
    ensure_motor_ready();
    // 设置方向GPIO电平
    right_motor_dir->gpio_level_set(dir ? GPIO_HIGH : GPIO_LOW);
    // 设置PWM占空比（限幅后转换为无符号整数）
    right_motor_pwm->atim_pwm_set_duty(static_cast<uint32_t>(clamp_pwm(duty)));
}

// ========== 电机禁用函数 ==========
void Motor_Disable1(void)
{
    if (!motor_initialized)
    {
        return;
    }
    // 设置PWM占空比为0
    left_motor_pwm->atim_pwm_set_duty(0);
    right_motor_pwm->atim_pwm_set_duty(0);
    // 禁用PWM输出
    left_motor_pwm->atim_pwm_disable();
    right_motor_pwm->atim_pwm_disable();
    // 标记未初始化
    motor_initialized = false;
}

// ========== 核心函数：电机控制主循环 ==========
void Motor_Control(void)
{
    ensure_motor_ready();

    // 调试日志计数器：每5次循环输出一次日志
    static uint32_t debug_log_divider = 0;

    // 1. 读取编码器值（左轮取负是因为安装方向）
    encoder_Left = -static_cast<int16_t>(left_motor_encoder->encoder_get_count());
    encoder_Right = static_cast<int16_t>(right_motor_encoder->encoder_get_count());

    // 2. 判断是否停止
    if (stop_flag == 1)
    {
        stop_motor_output();
    }
    else
    {
        // 设置巡航速度
        Speed_Goal_l = kDefaultCruiseSpeed;
        Speed_Goal_r = kDefaultCruiseSpeed;

        // 3. 【已注销转向环】直接使用巡航速度，不根据摄像头调整差速
        Motor_Diff_Pid1();
        // Diff_SpeedL_expect = Speed_Goal_l;
        // Diff_SpeedR_expect = Speed_Goal_r;

        // 4. 执行左右轮速度环PID控制
        Motor_PID_Left();
        Motor_PID_Right();
    }

    // 5. 输出调试日志（每5次循环）
    if (++debug_log_divider >= 5)
    {
        debug_log_divider = 0;
        const int center_error = ImageStatus.Det_True - ImageStatus.MiddleLine;
        printf("err=%4d  pidL=%6d  pidR=%6d  spdL=%4d  spdR=%4d\n",
               center_error,      // 转向误差（摄像头检测位置-中心线）
               Speed_PID_OUT_l,   // 左轮PID输出
               Speed_PID_OUT_r,   // 右轮PID输出
               encoder_Left,      // 左轮实际速度
               encoder_Right);    // 右轮实际速度
    }
}

// ========== 左轮速度环PID控制 ==========
void Motor_PID_Left(void)
{
    ensure_motor_ready();

    // 1. 获取左轮编码器速度
    Speed_Encoder_l = encoder_Left;
    
    // 2. 调用增量式PID计算，更新误差和输出
    Speed_Erro_l = update_speed_pid(Diff_SpeedL_expect,   // 目标速度（经过转向调整）
                                    Speed_Encoder_l,      // 实际速度
                                    Speed_P_l,            // P参数
                                    Speed_I_l,            // I参数
                                    Speed_D_l,            // D参数
                                    Speed_Lasterro_l,     // 上一次误差（状态变量）
                                    Speed_Preverro_l,     // 上上次误差（状态变量）
                                    Speed_PID_OUT_l);     // PID输出（状态变量）

    // 3. 根据PID输出符号决定方向并输出PWM
    if (Speed_PID_OUT_l >= 0)
    {
        // 正输出：前进方向
        Left_Motor_Pwm1(Speed_PID_OUT_l, kLeftForwardDir);
    }
    else
    {
        // 负输出：后退方向（取绝对值输出）
        Left_Motor_Pwm1(-Speed_PID_OUT_l, !kLeftForwardDir);
    }
}

// ========== 右轮速度环PID控制（与左轮对称） ==========
void Motor_PID_Right(void)
{
    ensure_motor_ready();

    // 1. 获取右轮编码器速度
    Speed_Encoder_r = encoder_Right;
    
    // 2. 调用增量式PID计算
    Speed_Erro_r = update_speed_pid(Diff_SpeedR_expect,
                                    Speed_Encoder_r,
                                    Speed_P_r,
                                    Speed_I_r,
                                    Speed_D_r,
                                    Speed_Lasterro_r,
                                    Speed_Preverro_r,
                                    Speed_PID_OUT_r);

    // 3. 根据PID输出符号决定方向并输出PWM
    if (Speed_PID_OUT_r >= 0)
    {
        Right_Motor_Pwm1(Speed_PID_OUT_r, kRightForwardDir);
    }
    else
    {
        Right_Motor_Pwm1(-Speed_PID_OUT_r, !kRightForwardDir);
    }
}

// ========== 核心函数：转向环差速PID控制 ==========
// 根据摄像头检测的位置误差，计算左右轮的目标速度差
void Motor_Diff_Pid1(void)
{
    // 1. 如果目标速度为0，直接返回
    if (Speed_Goal_l == 0 && Speed_Goal_r == 0)
    {
        Diff_SpeedL_expect = 0;
        Diff_SpeedR_expect = 0;
        last_turn_error = 0.0f;
        return;
    }

    // 2. 计算转向误差：摄像头检测位置 - 中心线
    float turn_error = ImageStatus.Det_True - (float)ImageStatus.MiddleLine;
    
    // 3. 死区处理：误差在-3~3之间视为0，避免微小抖动
    if (turn_error > -3.0f && turn_error < 3.0f)
    {
        turn_error = 0;
    }

    // 4. 分段P参数：根据误差大小调整灵敏度
    float current_Kp = Diff_Kp;
    if (turn_error > -7.0f && turn_error < 7.0f)
    {
        // 小误差：降低灵敏度（60%），避免过度修正
        current_Kp = Diff_Kp * 0.3f;
    }
    else if (turn_error > -13.0f && turn_error < 13.0f)
    {
        // 中误差：正常灵敏度（100%）
        current_Kp = Diff_Kp * 6.5f;
    }
    // 大误差：使用默认灵敏度（100%以上，由Diff_Kp决定）

    // 5. 转向环PD计算：输出转向速度差
    float turn_output = current_Kp * turn_error + Diff_Kd * (turn_error - last_turn_error);
    last_turn_error = turn_error;

    // 6. 转向输出限幅：最大±50
    if (turn_output > 50.0f)
    {
        turn_output = 50.0f;
    }
    if (turn_output < -50.0f)
    {
        turn_output = -50.0f;
    }

    // 7. 计算基础速度（左右轮目标速度的平均值）
    const int base_speed_goal = (Speed_Goal_l + Speed_Goal_r) / 2;
    
    // 8. 弯道减速：误差越大，速度越低（系数0.8）
    int current_base_speed = base_speed_goal - static_cast<int>(my_abs(turn_error) * 0.6f);

    // 9. 弯道预判减速：检测到即将丢线时（OFFLine在5~25之间），提前减速
    if (ImageStatus.OFFLine > 3 && ImageStatus.OFFLine < 25)
{
    int predict_slow = 0;

    if (ImageStatus.OFFLine < 5)
    {
        predict_slow = static_cast<int>(ImageStatus.OFFLine * 0.9f);
    }
    else if (ImageStatus.OFFLine < 12)
    {
        predict_slow = static_cast<int>(ImageStatus.OFFLine * 1.4f);
    }
    else
    {
        predict_slow = static_cast<int>(ImageStatus.OFFLine * 1.7f);
    }

    current_base_speed -= predict_slow;
}

    // 10. 大弯/掉头减速：误差超过20时，进一步减速20
    if (my_abs(turn_error) > 18)
    {
        current_base_speed -= 40;
    }

    // 11. 最低速度保护：不低于5
    if (current_base_speed < 5)
    {
        current_base_speed = 5;
    }

    // 12. 差速限制：根据误差大小决定是否允许反转
    int max_turn;
    if (my_abs(turn_error) > 20)
    {
        // 大弯/掉头：允许一侧反转，反转幅度
        max_turn = current_base_speed + 50;
    }
    else
    {
        // 普通弯道：不反转，最低速度为0
        max_turn = current_base_speed + 30;
    }
    // 限幅转向输出
    if (turn_output > max_turn)
    {
        turn_output = max_turn;
    }
    if (turn_output < -max_turn)
    {
        turn_output = -max_turn;
    }

    // 13. 计算最终目标速度：基础速度 ± 转向差速
    // 左轮：基础速度 + 转向输出（turn_output>0时左轮加速，实现左转）
    // 右轮：基础速度 - 转向输出（turn_output>0时右轮减速，实现左转）
    Diff_SpeedL_expect = static_cast<int16_t>(current_base_speed + static_cast<int>(turn_output));
    Diff_SpeedR_expect = static_cast<int16_t>(current_base_speed - static_cast<int>(turn_output));

    // 14. 最终限幅：反转不超过-20，正转不超过1500
    if (Diff_SpeedL_expect < -20)
    {
        Diff_SpeedL_expect = -20;
    }
    if (Diff_SpeedR_expect < -20)
    {
        Diff_SpeedR_expect = -20;
    }
    if (Diff_SpeedL_expect > 1500)
    {
        Diff_SpeedL_expect = 1500;
    }
    if (Diff_SpeedR_expect > 1500)
    {
        Diff_SpeedR_expect = 1500;
    }
}