#include "zf_common_headfile.h"
#include "pwm.h"
#include "classifier.h"

extern float kp;
extern float kd;
extern float ki;

extern Mat square;
extern Mat grayImage;

extern int up_corner_sum;
extern int down_corner_sum;
extern int circle_save;
extern int circle_save_old;
extern int patch_flag;
extern int slow_speed;
extern int fast_speed;
extern int mid_speed;
extern int key_scan_gpio;
extern int accel_num[];
extern int gyro_num[];
extern int base_speed;
extern int left_expect_speed;
extern int right_expect_speed;
extern int zebra_count;
extern int zebra_cold_time;
extern int circle_flag_count;
extern int circle_save_line[10000];
extern int barrier_flag_count;
extern int barrier_save_line[10000];
extern int left_corner[100][2];
extern int right_corner[100][2];
extern float error_val;
extern float error_last;
extern int expect_speed;
extern int ramp_flag;
extern int16 encoder_left;
extern int16 encoder_right;
extern int line_chose;
extern int left_straight;
extern int right_straight;
extern int ramp_save;
extern int barrier_save;
extern int barrier_hold_way;
extern int barrier_hold_true;
extern float expect_gyro;
extern int start_flag;
// 10 2 12
float wp = 12;
float wi = 2;
float wd = 6;
float dif_speed = 0.85;
int zebra_hold = 50;
int le[3] = {0};
int re[3] = {0};
int pwm_left = 0;
int pwm_right = 0;
int pwm_servo = 5005;
int pwm_servo_true = 5005;
int error_compute()
{

   pwm_servo_true = 0;

   // return (imu_gyro_z * 0.01 * gp_true - error_val);
   imu_gyro_z = imu_get_raw(imu_file_path[GYRO_Z_RAW]);
   //cout << "imu_gyro_z: " << imu_gyro_z << endl;
   error_val = (imu_gyro_z - expect_gyro) / 100.0;
   float error_dif = error_val - error_last;

   pwm_servo_true = (int)(error_val * kp + error_dif * kd);

   return pwm_servo_true;
}

float kalx = 0; // 状态估计值

float kalp = 1.0; // 估计误差协方差
float kalq = 0.5; // 过程噪声
float kalr = 5;   // 测量噪声

/**
 * @brief 针对整数输入的卡尔曼滤波
 * @param measurement 整数测量值
 * @return 滤波后的整数估计值
 */
int kalman_filter_int(int measurement)
{
   // 1. 预测阶段
   kalp = kalp + kalq;

   // 2. 更新阶段
   // 计算卡尔曼增益
   float k = kalp / (kalp + kalr);

   // 更新估计值 (将 int 测量值转为 float 计算)
   kalx = kalx + k * ((float)measurement - kalx);

   // 更新误差协方差
   kalp = (1.0f - k) * kalp;

   // 3. 返回整数结果 (四舍五入)
   return (int)(kalx + 0.5f);
}

void speed_control(int16 left_speed, int16 right_speed, int left_goal, int right_goal)
{

   le[0] = left_goal - left_speed; // e(k)

   // 计算比例项 P = Kp * (e(k) - e(k-1))
   int lP = (int)(wp * (le[0] - le[1]));

   // 计算积分项 I = Ki * e(k)
   int lI = (int)(wi * le[0]);

   // 计算微分项 D = Kd * (e(k) - 2 * e(k-1) + e(k-2))
   int lD = (int)(wd * (le[0] - 2 * le[1] + le[2]));

   // 计算控制量 u(k) = P + I + D
   pwm_left += (int)((lP + lI + lD));

   // 更新误差历史
   le[2] = le[1]; // 更新 e(k-2)
   le[1] = le[0]; // 更新 e(k-1)

   // 右侧控制代码
   re[0] = right_goal - right_speed; // e(k)

   // 计算比例项 P = Kp * (e(k) - e(k-1))
   int rP = (int)(wp * (re[0] - re[1]));

   // 计算积分项 I = Ki * e(k)
   int rI = (int)(wi * re[0]);

   // 计算微分项 D = Kd * (e(k) - 2 * e(k-1) + e(k-2))
   int rD = (int)(wd * (re[0] - 2 * re[1] + re[2]));

   // 计算控制量 u(k) = P + I + D
   pwm_right += (int)((rP + rI + rD));

   // 更新误差历史
   re[2] = re[1]; // 更新 e(k-2)
   re[1] = re[0]; // 更新 e(k-1)

   if (pwm_left > 9990)
   {

      pwm_left = 9990;
   }
   if (pwm_left < -9990)
   {

      pwm_left = -9990;
   }
   if (pwm_right > 9990)
   {

      pwm_right = 9990;
   }
   if (pwm_right < -9990)
   {

      pwm_right = -9990;
   }

   if (pwm_left < 0)
   {

      gpio_set(LEFT_WHELL_GPIO, LEFT_BACKWARD);
      pwm_set_duty(LEFT_WHELL_PWM, -pwm_left);
   }
   else
   {

      gpio_set(LEFT_WHELL_GPIO, LEFT_FORWARD);
      pwm_set_duty(LEFT_WHELL_PWM, pwm_left);
   }
   if (pwm_right < 0)
   {

      gpio_set(RIGHT_WHELL_GPIO, RIGHT_BACKWARD);
      pwm_set_duty(RIGHT_WHELL_PWM, -pwm_right);
   }
   else
   {

      gpio_set(RIGHT_WHELL_GPIO, RIGHT_FORWARD);
      pwm_set_duty(RIGHT_WHELL_PWM, pwm_right);
   }
   // system("clear");
   /*cout << "pwm_left:  " << pwm_left << endl;
   cout << "pwm_right: " << pwm_right << endl;
   cout << "lerror:    " << lerror << endl;
   cout << "rerror:    " << rerror << endl; */
   // pwm_set_duty(RIGHT_WHELL_PWM, pwm_right);
}

void speed_control_reset()
{

   pwm_left = 0;
   pwm_right = 0;
   le[0] = le[1] = le[2] = 0;
   re[0] = re[1] = re[2] = 0;
}

void stop_car()
{

   left_expect_speed = right_expect_speed = 0;
   expect_speed = 0;
   start_flag = -1;

   gpio_set(LEFT_WHELL_GPIO, LEFT_FORWARD);
   gpio_set(RIGHT_WHELL_GPIO, RIGHT_FORWARD);
   pwm_set_duty(LEFT_WHELL_PWM, 0);
   pwm_set_duty(RIGHT_WHELL_PWM, 0);
   pwm_set_duty(POWER_TRANSFER, 0);

   save_all_photo();

   cout << "STOP" << endl;
   cout << "circle_flag_count: " << circle_flag_count << endl;

   for (int i = 0; i < circle_flag_count; i++)
   {

      cout << circle_save_line[i] << " ";
   }

   cout << endl;

   cout << "ramp_save: " << ramp_save << endl;

   cout << "barrier_flag_count: " << barrier_flag_count << endl;
   string barrier_str[6] = {"gun", "bomb", "first_aid_kit", "telescope", "armored_car", "ambulance"};
   //string barrier_str[6] = {"weapon-0", "weapon-1", "supplies-0", "supplies-1", "car-0", "car-1"};
   for (int i = 0; i < barrier_flag_count; i++)
   {

      // cout << barrier_save_line[i] << " ";
      cout << barrier_str[barrier_save_line[i]] << " ";
   }
   cout << endl;
   print_line_pixels();
   while (1)
   {

      key_scan_gpio = keyboard_scan_gpio();
      if (key_scan_gpio == ESC || key_scan_gpio == ENTER)
      {
         system("clear");
         break;
      }
   }
   pit_ms_clean();
   zebra_count = 0;
}