#ifndef __CONCTROL_HPP__
#define __CONCTROL_HPP__
#include  "zf_common_headfile.hpp"
#define pwm_limit (10000)
#define Dir_limit (2600)
#define Gyro_limit (800)
#define BLACK_SOFT_SPEED_DEFAULT (60) // 旧参数文件没有该项时使用的兼容默认值
#define CARD_STOP_SPEED_EPS    (4)  // 左右轮绝对速度均不超过此值才算接近静止
#define CARD_STOP_STABLE_TICKS (25) // 2ms速度采样下连续25次=约50ms停稳
#define CARD_SEQUENCE_COUNT    (5)  // 赛道内按出现顺序配置的卡片总数

// 卡片识别完成后的倒车阶段。倒车期间保持 recog_staute=1 锁住本次分类，
// 但暂不执行绕行；倒完并再次停稳后才进入 TURN_IN 固定角度阶段。
enum CardReversePhase
{
    CARD_REVERSE_IDLE = 0,
    CARD_REVERSE_DRIVING = 1,
    CARD_REVERSE_BRAKING = 2
};
#define ENCODER_LEFT_PATH           ZF_ENCODER_DIR_1
#define ENCODER_RIGHT_PATH           ZF_ENCODER_DIR_2


#define PWM_LEFT_PATH        ZF_PWM_MOTOR_1
#define DIR_LEFT_PATH        ZF_GPIO_MOTOR_2

#define PWM_RIGHT_PATH        ZF_PWM_MOTOR_2
#define DIR_RIGHT_PATH        ZF_GPIO_MOTOR_1
#define PWM_FAN_PATH     ZF_PWM_ESC_1    //负压引脚

extern struct pwm_info drv8701e_pwm_left_info;
extern struct pwm_info drv8701e_pwm_right_info;
extern struct pwm_info pwm_fan_info;


extern zf_driver_gpio  drv8701e_dir_left;
extern zf_driver_gpio  drv8701e_dir_right;
extern zf_driver_pwm   drv8701e_pwm_left;
extern zf_driver_pwm   drv8701e_pwm_right;
extern zf_driver_pwm   pwm_fan;
//==================速度控制
extern int base_speed;//基础速度
extern int curve_speed;//弯道基础速度(稳定弯道时用)
extern int def_speed;//偏转速度
extern int black_soft_speed;//仅发现二值黑块候选时的一级软限速（菜单 BkSp）
extern int slope_speed;//坡道速度
extern int slope_ju;//坡道判断距离
extern int TOF_ju;//TOF判定坡道的距离
extern int brick_dis_isp;//红砖积分标志位
extern int brick_real_dis;//红砖实际行进距离积分
extern int brick_ju;//红砖积分距离判定(大于阈值清零)
extern int Brick_handled;//上锁
extern int TOF_DIS;//TOF测得的距离
extern int slope_flag;//坡道标志位
//打角行参数
extern int base_turn_propect;//基础循迹打角行
extern int def_turn_propect;//偏转打角行
extern int cd_turn_propect;//绕行退出冷却期的前瞻行(用于回正/接上弯道循迹)
extern int halo_turn_propect;//环岛绕行专用前瞻行(菜单 EV_TpH)
extern int FAN_SET;//pwm设置值
extern int FAN_SET_EVA;//绕行(偏转)时单独的负压pwm设置值



extern int Skp; //偏转时的速度环参数
extern int Ski;
extern int Skd;
extern int brake_force;//起刹主动刹车力度(超速时叠加反向输出,不依赖Ski)





//
extern float Gyro_Feedforward; // 全局前馈输出量
extern int Kf_EVA_milli;       // 绕行前馈系数, 实际增益 = Kf_EVA_milli / 1000.0
extern int start_flag;//启动标志位
extern int speed_left_now;      //左轮本次速度
extern int speed_right_now;     //右轮本次速度
extern int speed_car;//车辆速度(左右轮速度平均)
extern int speed_left_error;      //左轮本次速度误差
extern int speed_right_error;     //右轮本次速度误差
extern int speed_left_error_last;     //左轮上次速度误差
extern int speed_right_error_last;    //右轮上次速度误差
extern int speed_left_error_integral;    //左轮速度误差积分
extern int speed_right_error_integral;   //右轮速度误差积分
extern int speed_left_error_last_last;     //左轮上上次速度误差
extern int speed_right_error_last_last;    //右轮上上次速度误差
extern int kpL_speed_close , kiL_speed_close , kdL_speed_close ;      //左轮闭环参数kp,ki,kd
extern int kpR_speed_close , kiR_speed_close , kdR_speed_close ;      //右轮闭环参数kp,ki,kd
extern int speed_left_output,speed_right_output;    //闭环输出（pwm输入）
extern int speed_left_target,speed_right_target;    //左右轮的目标速度
extern int groyz_now;
extern int halt_time;//刹车时间
extern int halt_sp;//刹车速度
extern int ED;

//定义串级pid控制参数
//---------------------------------------------------------------------------------
extern int Dir_kp,Dir_ki,Dir_kd;    //定义转向外环pid参数（方向环）
extern int Dir_Lkp_h,Dir_Lki_h,Dir_Lkd_h;//定义左环岛时刻外环pid参数
extern int Dir_Rkp_h,Dir_Rki_h,Dir_Rkd_h;//定义右环岛时刻外环pid参数
extern int Dir_kp_EVA,Dir_kd_EVA;//偏转(直道)方向环单独参数
extern int Dir_kp_EVA_W,Dir_kd_EVA_W;//偏转(弯道)方向环单独参数
extern int Dir_kp_EVA_H,Dir_kd_EVA_H;//偏转(环岛)方向环单独参数
extern int evade_curve;//绕行场景:0=直道(EV_*Z) 1=弯道(EV_*W) 2=环岛(EV_*H),识别时锁定
extern int evade_reverse;//绕行弯向:1=弯道逆绕行(识别时锁定,前瞻行提前防冲出)
extern int turn_propect_h;  //环岛的单独打角行
extern int Dir_SWP,Dir_SWI,Dir_SWD;//偏转绕行单独参数
extern int Gyro_kp,Gyro_ki,Gyro_kd; //定义转向内环pid参数（角速度环）
extern int Gyro_kp_EVA,Gyro_kd_EVA;//偏转(直道)角速度环
extern int Gyro_kp_EVA_W,Gyro_kd_EVA_W;//偏转(弯道)角速度环
extern int Gyro_kp_EVA_H,Gyro_kd_EVA_H;//偏转(环岛)角速度环单独参数
extern int gyro_out_lpf;//角速度环输出一阶低通系数(×0.01=α,100=不滤)
extern int Dir_output;  //定义外环输出（方向环）
extern int Gyro_error; //陀螺仪本次误差
extern int Gyro_error_integral;//陀螺仪误差积分
extern int Gyro_error_last;    //陀螺仪上次误差
extern int Gyro_output; //定义内环输出（角速度环）
extern int enable_gyro_flag;
extern float gyro_angle_z_add;
extern float Evade_angle_z;
extern int real_speed;//车辆实际目标速度
extern int evade_slow_lock;//1=检测到红块,减速&负压增强 锁定中(speed_decision维护,带防抖)
extern int pre_brake;//识别前期刹车latch(主循环 update_pre_brake 按检测帧维护)
extern int pre_brake_frames;//起刹后检测帧数；正式输入满足后还需等待编码器停稳才允许识别
extern int pre_brake_giveup;//预刹超距离放弃锁
extern int pre_brake_dis;//本次预刹累计距离
extern volatile int card_stop_request;//卡片已满足NCNN输入条件，1=强制两轮停车并保持
extern volatile int card_vehicle_stopped;//编码器连续确认左右轮已停稳
extern volatile int card_reverse_phase;//0空闲 1等速倒车 2到距后制动等待停稳
extern volatile int card_reverse_stopped;//倒车结束后编码器连续确认停稳
extern int card_reverse_target_dis;//识别后倒车目标距离，0=关闭（菜单 RvDis）
extern int card_reverse_speed;//倒车目标速度的绝对值（菜单 RvSp）
extern volatile int card_reverse_real_dis;//本次编码器累计的实际倒车距离
extern int zebra_stop_target_count;//ZbStop=n时第n+1次斑马线事件停车，<0关闭
extern int card_stop_target_count;//第m个完成分类的卡片停车，<=0关闭（菜单 CdStop）
extern int zebra_detected_count;//本次运行已确认的斑马线次数
extern int card_classified_count;//本次运行已完成分类的卡片数
// 识别前刹车安全兜底距离：起刹后走满此距离仍没到 DETECT_START_DIS(没识别)→强制退出减速恢复正常循迹。
// 单位同 EVADE_COOLDOWN_DIS(距离积分/100)，实车按需调大调小。
#define PRE_BRAKE_MAX_DIS 200

extern int Evade_Kp;   // 避障直通 P：决定偏转力度（给大点，比如 30~60）
extern int Evade_KP2 ;  //爆发力二次项
extern int Evade_Kd;   // 避障直通 D：抵抗图像突变引起的车头抽搐
extern int ESP_D;  

extern int card_recognition_cooldown_100ms;
extern volatile int card_recognition_cooldown_ticks;
extern int card_recognition_disable;//卡片识别总开关：0=正常识别，1=关闭赛道卡片识别（菜单 CdOff）
extern int card_sequence_enable[CARD_SEQUENCE_COUNT];//Card1~Card5：0=跳过，1=进入正常识别

void start_card_recognition_cooldown(void);
void update_card_recognition_cooldown(void);
int card_sequence_recognition_allowed(void);//当前/下一张卡片是否允许识别；超过Card5固定返回0
void update_card_sequence_gate(void);//主循环中在色块检测后、update_pre_brake前调用
void Dir_PID(void);     //方向环控制（外环）
void Gyro_PID(void);    //角速度环控制（内环）
void PWM_FAN_SET(int pwm);
void speed_mensure();//速度测量
void update_card_stop_feedback();//2ms调用：依据左右轮实际速度确认是否连续停稳
void update_card_reverse_feedback();//2ms调用：倒车到距制动后确认是否再次停稳
void gyroz_read();//IMU角速度读取
void Motor_pwm_set(int pwm_left,int pwm_right);//设置pwm正反转
void speed_close_PID(int speed_left_target,int speed_right_target);  //输入为目标速度
void speed_close_PID_INC(int speed_left_target, int speed_right_target);//增量式PID
void Distance_interial();//距离积分函数
void speed_decision();//速度决策函数
void update_pre_brake(void);//识别前期刹车latch维护(主循环每检测帧调用，位于Detect_process之前)
void reset_run_stop_counters(void);//每次从首页启动时清零斑马线/卡片停车计数
void reset_control_runtime_state(void);//发车前清除PID、距离积分、卡片/红砖/坡道控制状态
void update_zebra_stop_counter(void);//主循环调用：斑马线事件去重、计数及到数停车
void update_zebra_delayed_stop(void);//2ms中断调用：目标斑马线出现后延时1秒停车
void turn_propect_decision();//打角行决策函数
void gyro_integrate();
void Get_DIS();//测距函数
#endif
