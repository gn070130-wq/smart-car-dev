#include "zf_common_headfile.hpp"


struct pwm_info drv8701e_pwm_left_info;
struct pwm_info drv8701e_pwm_right_info;
struct pwm_info pwm_fan_info;

zf_driver_gpio  drv8701e_dir_left(DIR_LEFT_PATH, O_RDWR);
zf_driver_gpio  drv8701e_dir_right(DIR_RIGHT_PATH, O_RDWR);
zf_driver_pwm   drv8701e_pwm_left(PWM_LEFT_PATH);
zf_driver_pwm   drv8701e_pwm_right(PWM_RIGHT_PATH);
zf_driver_pwm   pwm_fan(PWM_FAN_PATH);

//==================速度控制
int base_speed=0;//基础速度
int curve_speed=0;//弯道基础速度(稳定弯道时用)
int def_speed=0;//偏转速度
int black_soft_speed=BLACK_SOFT_SPEED_DEFAULT;//黑块候选一级软限速，参数页5 BkSp
int slope_speed=40;//坡道速度
int slope_ju=0;//坡道判断距离
int TOF_ju=0;//TOF判定坡道的距离
int pre_brake = 0;        // 识别前期刹车latch(update_pre_brake 按检测帧维护, speed_decision 读取)
int pre_brake_giveup = 0; // 兜底放弃标志:起刹后走满 PRE_BRAKE_MAX_DIS 仍没识别 → 放弃刹车恢复正常循迹
int pre_brake_dis = 0;    // 起刹后行进距离积分(Distance_interial 累加,单位同 EVADE_COOLDOWN_DIS)
int pre_brake_frames = 0; // 起刹后已经过的检测帧数(pre_brake==1期间累加)。
volatile int card_stop_request = 0;  // 满足正式NCNN输入条件后锁存，直到分类完成
volatile int card_vehicle_stopped = 0; // 编码器确认停稳后置1，分类完成/请求取消后清零
volatile int card_reverse_phase = CARD_REVERSE_IDLE;
volatile int card_reverse_stopped = 0;
int card_reverse_target_dis = 0; // 0=关闭；由菜单 RvDis 设置
int card_reverse_speed = 0;      // 取绝对值作为倒车速度；由菜单 RvSp 设置
volatile int card_reverse_real_dis = 0;
int zebra_stop_target_count = 0; // ZbStop=n时第n+1次斑马线事件停车；负数关闭
int card_stop_target_count = 0;  // 第m个完成分类的卡片停车；0=关闭
int zebra_detected_count = 0;
int card_classified_count = 0;
int card_recognition_cooldown_100ms = 10; // CdCool=10：有效卡片分类后冷却1秒；0=关闭
volatile int card_recognition_cooldown_ticks = 0;
int card_recognition_disable = 0; // CdOff：0=正常识别卡片，1=关闭赛道卡片识别
int card_sequence_enable[CARD_SEQUENCE_COUNT] = {1, 1, 1, 1, 1};

// 卡片编号按“完整红色验证首次输出 Target_flag=1”的事件计数。
// 同一物理卡片会连续出现多帧，因此必须锁存当前编号，并在候选连续消失后才允许计下一张。
static int card_sequence_seen_count = 0;
static int card_sequence_active = 0;
static int card_sequence_active_index = -1;
static int card_sequence_active_allow = 1;
static int card_sequence_missing_frames = 0;
static int card_sequence_workflow_started = 0;
static const int CARD_SEQUENCE_REARM_FRAMES = 5;

// 跨发车周期的控制器内部状态。集中放在文件作用域，才能在重新发车前彻底清零。
static int card_stop_stable_ticks = 0;
static int card_reverse_stable_ticks = 0;
static int brake_debug_count = 0;
static float obstacle_feedforward_lpf = 0.0f;
static float gyro_speed_filtered = 0.0f;
static int distance_total_pian = 0;
static int distance_total_brick = 0;
static int distance_total_slope = 0;
static int brick_cooldown_active = 0;
static int brick_cooldown_total = 0;
static int brick_cooldown_real_dis = 0;
static int distance_total_reverse = 0;
static int distance_total_prebrake = 0;
static float speed_command_lpf = 0.0f;
static int last_card_evade_distance_phase = CARD_EVADE_IDLE;
static int card_evade_turn_stable_ticks = 0;
static int card_evade_turn_timeout_ticks = 0;
static int gyro_kalman_initialized = 0;
static float gyro_kalman_estimate = 0.0f;
static float gyro_kalman_error_cov = 1.0f;

static const float CARD_EVADE_YAW_DT = 0.0002f; //沿用当前IMU /5后的角度积分标度
static const float CARD_EVADE_TURN_TOLERANCE = 1.5f;
static const int CARD_EVADE_TURN_STABLE_TICKS = 10; //方向环8ms一次，约80ms稳定确认
static const int CARD_EVADE_TURN_TIMEOUT_TICKS = 375; //方向环8ms一次，约3秒兜底

// 同一条斑马线会连续出现多帧。必须先连续离开若干帧才重新布防，避免检测抖动
// 把同一次过线重复计为多圈。
static int zebra_event_armed = 1;
static int zebra_clear_frames = 0;
static const int ZEBRA_REARM_CLEAR_FRAMES = 5;
static volatile int zebra_stop_delay_active = 0;
static volatile int zebra_stop_delay_ticks = 0;
static const int ZEBRA_STOP_DELAY_TICKS = 500; // 2ms × 500 = 1秒

void reset_run_stop_counters(void)
{
    zebra_detected_count = 0;
    card_classified_count = 0;
    zebra_clear_frames = 0;
    zebra_stop_delay_active = 0;
    zebra_stop_delay_ticks = 0;
    card_recognition_cooldown_ticks = 0;
    card_sequence_seen_count = 0;
    card_sequence_active = 0;
    card_sequence_active_index = -1;
    card_sequence_active_allow = 1;
    card_sequence_missing_frames = 0;
    card_sequence_workflow_started = 0;

    for (int i = 0; i < CARD_SEQUENCE_COUNT; ++i)
    {
        card_sequence_enable[i] = (card_sequence_enable[i] == 1) ? 1 : 0;
    }

    if (card_recognition_disable == 0)
    {
        printf("卡片识别序列: %d%d%d%d%d (1=识别,0=直接通过)\n",
               card_sequence_enable[0], card_sequence_enable[1],
               card_sequence_enable[2], card_sequence_enable[3],
               card_sequence_enable[4]);
    }
    else
    {
        printf("卡片识别总开关已关闭: CdOff=1\n");
    }

    if (card_recognition_disable == 1)
    {
        pre_brake = 0;
        pre_brake_hint = 0;
        pre_brake_giveup = 0;
        pre_brake_frames = 0;
        pre_brake_dis = 0;
        card_stop_request = 0;
        card_vehicle_stopped = 0;
        card_reverse_phase = CARD_REVERSE_IDLE;
        card_reverse_stopped = 0;
        card_reverse_real_dis = 0;
        recog_staute = 0;
        Detect_dis_isp = 0;
        Detect_real_dis = 0;
        evade_cooldown_active = 0;
        card_evade_phase = CARD_EVADE_IDLE;
        card_evade_phase_dis = 0;
        card_evade_relative_yaw = 0.0f;
        card_evade_target_yaw = 0.0f;
        is_recognizing_now = 0;
        Target_flag = 0;
    }
    // 若上一次正停在斑马线上，重新启动后先等车辆离开，不能立刻把同一条线再计一次。
    zebra_event_armed = (banmaxian_flag == 0) ? 1 : 0;
}

void start_card_recognition_cooldown(void)
{
    if (card_recognition_cooldown_100ms <= 0)
    {
        card_recognition_cooldown_ticks = 0;
        return;
    }

    // 100ms = 50个2ms节拍；限制乘法上界，避免异常菜单值造成整型溢出。
    int units = card_recognition_cooldown_100ms;
    const int max_units = 0x7fffffff / 50;
    if (units > max_units) units = max_units;
    card_recognition_cooldown_ticks = units * 50;
}

void update_card_recognition_cooldown(void)
{
    if (card_recognition_cooldown_ticks > 0)
    {
        card_recognition_cooldown_ticks--;
    }
}

// 清除“本张卡片”可能留下的所有入口状态，使配置为0的卡片保持正常速度直接通过。
// 红砖状态不在这里清除；调用方只会在 Brick_flag!=1 时进入。
static void suppress_current_card_recognition(void)
{
    Target_flag = 0;
    Red_flag = 0;
    br_found = 0;
    target_y2 = 0;
    final_crop_img.release();

    pre_brake_hint = 0;
    pre_brake = 0;
    pre_brake_giveup = 0;
    pre_brake_frames = 0;
    pre_brake_dis = 0;
    is_recognizing_now = 0;
    recog_cnt = 0;
    card_stop_request = 0;
    card_vehicle_stopped = 0;
    evade_slow_lock = 0;

    // 预刹起始边沿可能已经给速度积分写入负偏置；跳过卡片时一并撤销，避免继续拖速。
    speed_left_error_integral = 0;
    speed_right_error_integral = 0;
}

int card_sequence_recognition_allowed(void)
{
    if (card_recognition_disable == 1) return 0;

    if (card_sequence_active == 1)
    {
        // active_index=-1 专门表示“Card5之后的额外候选”，永远不允许识别。
        if (card_sequence_active_index < 0 ||
            card_sequence_active_index >= CARD_SEQUENCE_COUNT)
        {
            return 0;
        }

        return (card_sequence_active_allow == 1 &&
                card_sequence_enable[card_sequence_active_index] == 1) ? 1 : 0;
    }

    // 前五张已经全部编号后，后续任何候选都不再进入识别。
    if (card_sequence_seen_count < 0 ||
        card_sequence_seen_count >= CARD_SEQUENCE_COUNT)
    {
        return 0;
    }

    return (card_sequence_enable[card_sequence_seen_count] == 1) ? 1 : 0;
}

void update_card_sequence_gate(void)
{
    // 编号只属于一次正式运行；停车调菜单/观察图像时不累计。
    if (car_running != 1) return;

    if (card_recognition_disable == 1)
    {
        card_sequence_active = 0;
        card_sequence_active_index = -1;
        card_sequence_active_allow = 0;
        card_sequence_missing_frames = 0;
        card_sequence_workflow_started = 0;
        return;
    }

    // CardN=1 一旦进入停车/分类/倒车/绕行链，就由现有状态机和冷却保证不会重复识别。
    // 整套流程结束后可直接释放序号锁，不再额外等待空画面，避免下一张刚出现时仍沿用上一编号。
    if (card_sequence_active == 1 && card_sequence_workflow_started == 1 &&
        card_stop_request == 0 && recog_staute == 0 &&
        card_reverse_phase == CARD_REVERSE_IDLE &&
        evade_cooldown_active == 0)
    {
        card_sequence_active = 0;
        card_sequence_active_index = -1;
        card_sequence_active_allow = 1;
        card_sequence_missing_frames = 0;
        card_sequence_workflow_started = 0;
    }

    const bool detector_window_open =
        (banmaxian_flag != 1) &&
        (evade_cooldown_active != 1) &&
        (card_reverse_phase == CARD_REVERSE_IDLE) &&
        (brick_dis_isp == 0);

    // raw_target 必须在本函数清 Target_flag 之前保存；br_found 用于抵抗完整红色验证的短暂闪断。
    const bool raw_target = detector_window_open && (Target_flag == 1);
    const bool supporting_candidate =
        detector_window_open && (br_found == 1) && (Brick_flag != 1);
    const bool current_card_workflow =
        (pre_brake == 1) || (card_stop_request == 1) ||
        (recog_staute == 1) || (card_reverse_phase != CARD_REVERSE_IDLE) ||
        (evade_cooldown_active == 1);

    // 只有空闲状态才能给一张新卡片编号。正在停车、分类、倒车、绕行或冷却时，
    // 画面中的残留目标不能被当成下一张卡片。
    if (raw_target && card_sequence_active == 0 &&
        recog_staute == 0 && card_stop_request == 0 &&
        card_reverse_phase == CARD_REVERSE_IDLE &&
        evade_cooldown_active == 0)
    {
        card_sequence_active = 1;
        card_sequence_missing_frames = 0;
        card_sequence_workflow_started = 0;

        if (card_sequence_seen_count >= 0 &&
            card_sequence_seen_count < CARD_SEQUENCE_COUNT)
        {
            card_sequence_active_index = card_sequence_seen_count;
            card_sequence_seen_count++;
            card_sequence_active_allow =
                (card_sequence_enable[card_sequence_active_index] == 1) ? 1 : 0;
            printf("检测到第%d张卡片: Card%d=%d -> %s\n",
                   card_sequence_active_index + 1,
                   card_sequence_active_index + 1,
                   card_sequence_active_allow,
                   card_sequence_active_allow ? "正常识别" : "不识别，直行\n");
        }
        else
        {
            // 前五张已经全部编号：计数固定停在5，额外候选只建立“跳过锁”，
            // 不再继续累加第6、7……张，也绝不读取数组范围以外的配置。
            card_sequence_seen_count = CARD_SEQUENCE_COUNT;
            card_sequence_active_index = -1;
            card_sequence_active_allow = 0;
            printf("检测到Card5之后的额外卡片候选\n直行\n");
        }
    }

    if (card_sequence_active == 1)
    {
        if (card_sequence_active_allow == 1 &&
            (card_stop_request == 1 || recog_staute == 1 ||
             card_reverse_phase != CARD_REVERSE_IDLE ||
             evade_cooldown_active == 1))
        {
            card_sequence_workflow_started = 1;
        }

        if (raw_target || supporting_candidate || current_card_workflow)
        {
            card_sequence_missing_frames = 0;
        }
        else
        {
            if (card_sequence_missing_frames < CARD_SEQUENCE_REARM_FRAMES)
                card_sequence_missing_frames++;

            if (card_sequence_missing_frames >= CARD_SEQUENCE_REARM_FRAMES)
            {
                card_sequence_active = 0;
                card_sequence_active_index = -1;
                card_sequence_active_allow = 1;
                card_sequence_missing_frames = 0;
                card_sequence_workflow_started = 0;
            }
        }
    }

    // 当前还没有完整确认卡片时，也按“下一张”的配置提前决定是否允许预刹/黑块软限速。
    // 这样 CardN=0 不会先被 pre_brake_hint 或 BkSp 拖慢，随后才发现需要跳过。
    int pending_allow = card_sequence_recognition_allowed();

    if (pending_allow == 0 && Brick_flag != 1 &&
        (raw_target || supporting_candidate || pre_brake_hint == 1 || pre_brake == 1))
    {
        suppress_current_card_recognition();
    }
}

void update_zebra_stop_counter(void)
{
    if (car_running != 1) return;
    // 已锁存目标圈数后只等待1秒定时器，不再接受新的斑马线计数。
    if (zebra_stop_delay_active == 1) return;

    if (banmaxian_flag == 1)
    {
        zebra_clear_frames = 0;
        if (zebra_event_armed == 1)
        {
            zebra_event_armed = 0;
            zebra_detected_count++;

            if (zebra_stop_target_count >= 0)
            {
                int zebra_stop_event = zebra_stop_target_count + 1;
                printf("斑马线计数: %d/%d（ZbStop=%d，对应%d圈）\n",
                       zebra_detected_count, zebra_stop_event,
                       zebra_stop_target_count, zebra_stop_event);
                if (zebra_detected_count >= zebra_stop_event)
                {
                    zebra_stop_delay_ticks = 0;
                    zebra_stop_delay_active = 1;
                    printf("已识别第%d次斑马线，完成%d圈（配置ZbStop=%d），1秒后停车\n",
                           zebra_detected_count, zebra_stop_event,
                           zebra_stop_target_count);
                }
            }
        }
        return;
    }

    if (zebra_event_armed == 0)
    {
        if (zebra_clear_frames < ZEBRA_REARM_CLEAR_FRAMES) zebra_clear_frames++;
        if (zebra_clear_frames >= ZEBRA_REARM_CLEAR_FRAMES)
        {
            zebra_event_armed = 1;
            zebra_clear_frames = 0;
        }
    }
}

void update_zebra_delayed_stop(void)
{
    if (zebra_stop_delay_active == 0) return;

    // 其他安全逻辑或人工按键已停车时，取消尚未执行的延时请求。
    if (car_running != 1)
    {
        zebra_stop_delay_active = 0;
        zebra_stop_delay_ticks = 0;
        return;
    }

    if (zebra_stop_delay_ticks < ZEBRA_STOP_DELAY_TICKS) zebra_stop_delay_ticks++;
    if (zebra_stop_delay_ticks >= ZEBRA_STOP_DELAY_TICKS)
    {
        zebra_stop_delay_active = 0;
        zebra_stop_delay_ticks = 0;
        car_running = 0;
        speed_left_target = 0;
        speed_right_target = 0;
        printf("停车原因: 目标斑马线识别后已继续行驶1秒\n");
    }
}

int FAN_SET;//pwm设置值
int FAN_SET_EVA;//绕行(偏转)时单独的负压pwm设置值
//打角行参数
int base_turn_propect=0;//基础循迹打角行
int def_turn_propect=0;//偏转打角行
int cd_turn_propect=0;//绕行退出冷却期的前瞻行(用于回正/接上弯道循迹)
int halo_turn_propect=0;//环岛绕行专用前瞻行(菜单 EV_TpH):环岛曲率比普通弯道紧,常需比 CDTp 看得更近
int TOF_DIS=0;//TOF测得的距离
int slope_flag=0;//坡道标志位
int slope_process_dis=0;//坡道处理距离积分判定
int groyz_now;
int groyy_now;
int filter_spL[5];    //左轮滤波数组  非全局变量
int filter_spR[5];    //右轮滤波数组
int start_flag;//启动标志位
int speed_left_now=0;      //左轮本次速度
int speed_right_now=0;     //右轮本次速度
int speed_car=0;//车辆速度(左右轮速度平均)
int speed_left_error=0;      //左轮本次速度误差
int speed_right_error=0;     //右轮本次速度误差
int speed_left_error_integral=0;    //左轮速度误差积分
int speed_right_error_integral=0;   //右轮速度误差积分
int speed_left_error_last=0;     //左轮上次速度误差
int speed_right_error_last=0;    //右轮上次速度误差
int speed_left_error_last_last=0;     //左轮上上次速度误差
int speed_right_error_last_last=0;    //右轮上上次速度误差
int kpL_speed_close=14 , kiL_speed_close =3 , kdL_speed_close=0;      //左轮闭环参数kp,ki,kd
int kpR_speed_close=14 , kiR_speed_close =3 , kdR_speed_close=0;      //右轮闭环参数kp,ki,kd
int speed_left_output,speed_right_output=0;    //闭环输出（pwm输入）
int speed_left_target,speed_right_target=0;    //左右轮的目标速度

//定义串级pid控制参数
//---------------------------------------------------------------------------------（串级方案）
int Dir_kp=12,Dir_ki=0,Dir_kd=6;    //定义转向外环pid参数（方向环）
int Dir_Lkp_h=1,Dir_Lki_h=0,Dir_Lkd_h=0;//定义左环岛时刻外环pid参数
int Dir_Rkp_h=1,Dir_Rki_h=0,Dir_Rkd_h=0;//定义右环岛时刻外环pid参数
int Dir_kp_EVA=1,Dir_kd_EVA=0;//偏转(直道)方向环单独参数
int Dir_kp_EVA_W=1,Dir_kd_EVA_W=0;//偏转(弯道)方向环单独参数
int Dir_kp_EVA_H=1,Dir_kd_EVA_H=0;//偏转(环岛)方向环单独参数(菜单 EV_DkpH/EV_DkdH)
int evade_curve=0;//绕行时锁定的场景:0=直道(EV_*Z) 1=弯道(EV_*W) 2=环岛(EV_*H)。识别瞬间一次性锁定,
                  //新绕行执行全程不变(环岛曲率比普通弯道紧得多,故单列一档独立参数)
int evade_reverse=0;//绕行时锁定的弯向:1=弯道逆绕行(左弯右偏/右弯左偏),识别时锁定,前瞻行提前防冲出
int Dir_SWP=1,Dir_SWI=0,Dir_SWD=0;//偏转绕行单独参数
int turn_propect_h=0;  //环岛的单独打角行
int Gyro_kp=12,Gyro_ki=0,Gyro_kd=8; //定义转向内环pid参数（角速度环）
int Gyro_kp_EVA=1,Gyro_kd_EVA=0;//偏转(直道)角速度环
int Gyro_kp_EVA_W=1,Gyro_kd_EVA_W=0;//偏转(弯道)角速度环
int Gyro_kp_EVA_H=1,Gyro_kd_EVA_H=0;//偏转(环岛)角速度环单独参数(菜单 EV_GkpH/EV_GkdH)
int gyro_out_lpf=80;//角速度环输出一阶低通系数(×0.01=α):100=不滤,越小越平滑但滞后越大。菜单 GOLpf 可调
int Dir_output=0;  //定义外环输出（方向环）   输出为目标角速度
int Gyro_output=0; //定义内环输出（角速度环）  输出为经过计算的实际角速度 转化为电机差速度
int Gyro_error; //陀螺仪本次误差
int Gyro_error_integral;//陀螺仪误差积分
int Gyro_error_last;    //陀螺仪上次误差
int real_speed=0;
void PWM_FAN_SET(int pwm)
{
    if(pwm <= 0)
    {
        pwm_fan.set_duty(0);
        return;
    }
    //限幅
    if(pwm<500)pwm=500;
    if(pwm>=950)pwm=950;
    //设置
    pwm_fan.set_duty(pwm);
}
void speed_mensure()
{
    // 定时器数值获取
    int tempL_pulse = 4*(int)encoder_left.get_count();
    
    int tempR_pulse = -4*(int)encoder_right.get_count();
   // printf("%d %d\n",tempL_pulse,tempR_pulse);
    // speed_left_now = tempL_pulse;
    // speed_right_now = tempL_pulse;
    //将脉冲赋值给滤波数组并更新数组(平滑滤波)
    filter_spL[0]=filter_spL[1];filter_spL[1]=filter_spL[2];filter_spL[2]=filter_spL[3];filter_spL[3]=filter_spL[4];    
    filter_spR[0]=filter_spR[1];filter_spR[1]=filter_spR[2];filter_spR[2]=filter_spR[3];filter_spR[3]=filter_spR[4];    
    filter_spL[4]=tempL_pulse;filter_spR[4]=tempR_pulse;
    // //滤波数组加权重赋值给速度
    speed_left_now= 0.6 * (0.1*filter_spL[0]+0.1*filter_spL[1]+0.1*filter_spL[2]+0.1*filter_spL[3]+0.6*filter_spL[4]);
    speed_right_now= 0.6 * (0.1*filter_spR[0]+0.1*filter_spR[1]+0.1*filter_spR[2]+0.1*filter_spR[3]+0.6*filter_spR[4]);
    speed_car=(int)((speed_left_now+speed_right_now)/2);
    // 定时器清零
    encoder_left.clear_count();
    encoder_right.clear_count();
    // printf("spl=%d,spr=%d\n",speed_left_now,speed_right_now);
    //printf("speedmeasure");
}

// 卡片停车反馈：停车请求期间，左右轮连续约50ms处于低速死区才允许NCNN开始推理。
// 不能用单次速度为0判断，因为2ms编码器采样在低速时可能偶发读到0。
void update_card_stop_feedback()
{
    if (card_stop_request == 0)
    {
        card_stop_stable_ticks = 0;
        card_vehicle_stopped = 0;
        return;
    }

    int abs_left  = (speed_left_now  >= 0) ? speed_left_now  : -speed_left_now;
    int abs_right = (speed_right_now >= 0) ? speed_right_now : -speed_right_now;

    if (abs_left <= CARD_STOP_SPEED_EPS && abs_right <= CARD_STOP_SPEED_EPS)
    {
        if (card_stop_stable_ticks < CARD_STOP_STABLE_TICKS) card_stop_stable_ticks++;
        if (card_stop_stable_ticks >= CARD_STOP_STABLE_TICKS) card_vehicle_stopped = 1;
    }
    else
    {
        card_stop_stable_ticks = 0;
        card_vehicle_stopped = 0;
    }
}

// 倒车达到设定距离后先把两轮目标置零，再连续确认实际轮速已经归零。
// 只有确认停稳，主线程才会释放倒车锁并开始正向绕行，避免电机瞬间反向冲击。
void update_card_reverse_feedback()
{
    if (card_reverse_phase != CARD_REVERSE_BRAKING)
    {
        card_reverse_stable_ticks = 0;
        card_reverse_stopped = 0;
        return;
    }

    if (card_reverse_stopped == 1) return;

    int abs_left  = (speed_left_now  >= 0) ? speed_left_now  : -speed_left_now;
    int abs_right = (speed_right_now >= 0) ? speed_right_now : -speed_right_now;

    if (abs_left <= CARD_STOP_SPEED_EPS && abs_right <= CARD_STOP_SPEED_EPS)
    {
        if (card_reverse_stable_ticks < CARD_STOP_STABLE_TICKS) card_reverse_stable_ticks++;
        if (card_reverse_stable_ticks >= CARD_STOP_STABLE_TICKS) card_reverse_stopped = 1;
    }
    else
    {
        card_reverse_stable_ticks = 0;
    }
}

int gyro_z_kalman_filter(int raw_gyro_z)
{
    const float process_noise = 5.0f;   // 越大响应越快
    const float measure_noise = 25.0f;  // 越大滤波越稳

    if (gyro_kalman_initialized == 0)
    {
        gyro_kalman_estimate = static_cast<float>(raw_gyro_z);
        gyro_kalman_error_cov = 1.0f;
        gyro_kalman_initialized = 1;
        return raw_gyro_z;
    }

    gyro_kalman_error_cov += process_noise;
    float kalman_gain = gyro_kalman_error_cov / (gyro_kalman_error_cov + measure_noise);
    gyro_kalman_estimate += kalman_gain * (static_cast<float>(raw_gyro_z) - gyro_kalman_estimate);
    gyro_kalman_error_cov = (1.0f - kalman_gain) * gyro_kalman_error_cov;

    return static_cast<int>((gyro_kalman_estimate >= 0.0f) ?
                            (gyro_kalman_estimate + 0.5f) :
                            (gyro_kalman_estimate - 0.5f));
}

void gyroz_read()//IMU角速度读取
{
    int raw_gyro_z =   (imu_dev.get_gyro_z()/5 );
    // groyz_now = gyro_z_kalman_filter(raw_gyro_z);
    groyz_now = raw_gyro_z; // 想临时关闭卡尔曼滤波时,改用这一行即可
    // groyy_now=-1*(imu_dev.get_gyro_y()/5);
    // int x=-1*(imu_dev.get_gyro_x()/5);
    // printf("goX=%d,goY=%d,goZ=%d\n",x,groyy_now,groyz_now);
    
    // printf("%d\n",groyz_now);
  
}
//电机闭环PID控制函数（位置式pid）
int L_speedmotor = 0;     //定义pwm输入值
int R_speedmotor = 0;
void Motor_pwm_set(int pwm_left,int pwm_right)//设置pwm正反转
{
    // if(pwm_left>3000||pwm_left<-3000)
    // {
    //     pwm_left = (pwm_left > 3000) ? 3000 : -3000;
    // }
    // if(pwm_right>3000||pwm_right<-3000)
    // {
    //     pwm_right = (pwm_right > 3000) ? 3000 : -3000;
    // }
    if(pwm_left>=0)
    {
        drv8701e_dir_left.set_level(1);                                      // DIR输出高电平
        drv8701e_pwm_left.set_duty(pwm_left);
    }
    if(pwm_left<0)
    {
        drv8701e_dir_left.set_level(0);                                      // DIR输出高电平
        drv8701e_pwm_left.set_duty(-1*pwm_left);
    }
    if(pwm_right>=0)
    {
        drv8701e_dir_right.set_level(0);                                      // DIR输出高电平
        drv8701e_pwm_right.set_duty(pwm_right);
    }
    if(pwm_right<0)
    {
        drv8701e_dir_right.set_level(1);                                      // DIR输出高电平
        drv8701e_pwm_right.set_duty(-1*pwm_right);
    }
        
}
int integral_limit=5000;//积分限幅
int Skp=1;
int Ski=0;
int Skd=0;
int brake_force=30;//起刹主动刹车力度:识别前期刹车超速时直接叠加的反向输出增益(越大刹越猛,不依赖Ski)
void speed_close_PID(int speed_left_target,int speed_right_target)  //输入为目标速度
{

        // 计算当前速度误差
        speed_left_error = speed_left_target - speed_left_now;
        speed_right_error = speed_right_target - speed_right_now;
        //速度误差积分
        speed_left_error_integral+=speed_left_error;
        speed_right_error_integral+=speed_right_error ;

        //速度误差双向限幅
        if(speed_left_error_integral>integral_limit)speed_left_error_integral=integral_limit;
        if(speed_left_error_integral<-1*integral_limit)speed_left_error_integral=-1*integral_limit;
        if(speed_right_error_integral>integral_limit)speed_right_error_integral=integral_limit;
        if(speed_right_error_integral<-1*integral_limit)speed_right_error_integral=-1*integral_limit;
        // 计算PID输出量
        // 软PID(Skp/Ski/Skd)用于识别前刹车和新绕行的转向/清障/并线阶段；
        // 其余(正常循迹/冷却)用正常速度环参数(kpL/kiL/kdL...)。
        if(pre_brake == 1 || card_stop_request == 1 ||
           (card_evade_phase >= CARD_EVADE_TURN_IN &&
            card_evade_phase <= CARD_EVADE_SETTLE))
        {   
            //printf("speed_error_pid");
            speed_left_output =  Skp  * speed_left_error +  speed_left_error_integral * Ski * 0.1 + Skd * (speed_left_error - speed_left_error_last );
            speed_right_output = Skp  * speed_right_error + speed_right_error_integral * Ski * 0.1 + Skd * (speed_right_error - speed_right_error_last);
            // [排查] 刹车期间每100个周期(≈200ms)打一次:看积分项是否真在输出反向刹
            if (pre_brake == 1 && ++brake_debug_count >= 100)
            {
                brake_debug_count = 0;
                // printf("刹车中 err=%d integ=%d out=%d\n", speed_left_error, speed_left_error_integral, speed_left_output);
            }
        }
        else
        {
            speed_left_output =  kpL_speed_close  * speed_left_error +  speed_left_error_integral*kiL_speed_close * 0.1 + kdL_speed_close * (speed_left_error - speed_left_error_last );
            speed_right_output = kpR_speed_close  * speed_right_error + speed_right_error_integral*kiR_speed_close * 0.1 + kdR_speed_close * (speed_right_error - speed_right_error_last);
        }
        // 起刹主动刹车：识别前期刹车(pre_brake)且实际速度高于目标(误差<0=超速)时，直接在PID输出上
        // 叠加反向刹车。这样先保证"真能减下来"，再由 speed_decision 的斜坡把进入绕行做顺。
        if (pre_brake == 1 || card_stop_request == 1)
        {
            if (speed_left_error  < 0) speed_left_output  += brake_force * speed_left_error;
            if (speed_right_error < 0) speed_right_output += brake_force * speed_right_error;
        }

        // 强制停车的零速死区：某一侧已经接近静止时立即撤掉该侧反向输出并清积分，
        // 防止最大反向制动越过零速后把轮子反向拉动、造成停车抽搐或原地转向。
        if (card_stop_request == 1 || card_reverse_phase == CARD_REVERSE_BRAKING)
        {
            int abs_left  = (speed_left_now  >= 0) ? speed_left_now  : -speed_left_now;
            int abs_right = (speed_right_now >= 0) ? speed_right_now : -speed_right_now;
            if (abs_left <= CARD_STOP_SPEED_EPS)
            {
                speed_left_output = 0;
                speed_left_error_integral = 0;
            }
            if (abs_right <= CARD_STOP_SPEED_EPS)
            {
                speed_right_output = 0;
                speed_right_error_integral = 0;
            }
        }
        // 更新误差记录
        speed_left_error_last_last = speed_left_error_last;
        speed_left_error_last = speed_left_error;
        speed_right_error_last_last = speed_right_error_last;
        speed_right_error_last = speed_right_error;
        // 对输出进行双向限幅处理
        if (speed_left_output > pwm_limit) {
            speed_left_output = pwm_limit;
        } else if (speed_left_output < -1*pwm_limit) {
            speed_left_output = -1*pwm_limit;
        }
        if (speed_right_output > pwm_limit) {
            speed_right_output = pwm_limit;
        } else if (speed_right_output < -1*pwm_limit) {
            speed_right_output = -1*pwm_limit;
        }
        // printf("l==%d, r==%d\n",speed_left_output,speed_right_output);
        Motor_pwm_set(speed_left_output,speed_right_output);
}

void speed_close_PID_INC(int speed_left_target, int speed_right_target)
{
    // ===================== 1. 计算当前速度误差 =====================
    speed_left_error  = speed_left_target  - speed_left_now;
    speed_right_error = speed_right_target - speed_right_now;

    // ===================== 2. 增量式 PI/PID 计算 =====================
    // ΔU = Kp * (e(k) - e(k-1))
    //    + Ki * e(k)
    //    + Kd * (e(k) - 2e(k-1) + e(k-2))

    float left_increment_f =
          kpL_speed_close * (speed_left_error - speed_left_error_last)
        + kiL_speed_close * speed_left_error * 0.1f
        + kdL_speed_close * (speed_left_error - 2 * speed_left_error_last + speed_left_error_last_last);

    float right_increment_f =
          kpR_speed_close * (speed_right_error - speed_right_error_last)
        + kiR_speed_close * speed_right_error *0.1f
        + kdR_speed_close * (speed_right_error - 2 * speed_right_error_last + speed_right_error_last_last);

    int speed_left_increment  = (int)left_increment_f;
    int speed_right_increment = (int)right_increment_f;

    // ===================== 3. 单次增量限幅 =====================
    // 2ms 速度环，建议先从 40~60 开始
    const int INCREMENT_LIMIT = 5000;

    if(speed_left_increment > INCREMENT_LIMIT)
        speed_left_increment = INCREMENT_LIMIT;
    else if(speed_left_increment < -INCREMENT_LIMIT)
        speed_left_increment = -INCREMENT_LIMIT;

    if(speed_right_increment > INCREMENT_LIMIT)
        speed_right_increment = INCREMENT_LIMIT;
    else if(speed_right_increment < -INCREMENT_LIMIT)
        speed_right_increment = -INCREMENT_LIMIT;

    // ===================== 4. 饱和保护 =====================
    // PWM 已经到上限，就不要继续往上加
    if(speed_left_output >= pwm_limit && speed_left_increment > 0)
        speed_left_increment = 0;

    if(speed_left_output <= -pwm_limit && speed_left_increment < 0)
        speed_left_increment = 0;

    if(speed_right_output >= pwm_limit && speed_right_increment > 0)
        speed_right_increment = 0;

    if(speed_right_output <= -pwm_limit && speed_right_increment < 0)
        speed_right_increment = 0;

    // ===================== 5. 累加输出 =====================
    speed_left_output  += speed_left_increment;
    speed_right_output += speed_right_increment;

    // ===================== 6. 最终输出限幅 =====================
    if(speed_left_output > pwm_limit)
        speed_left_output = pwm_limit;
    else if(speed_left_output < -pwm_limit)
        speed_left_output = -pwm_limit;

    if(speed_right_output > pwm_limit)
        speed_right_output = pwm_limit;
    else if(speed_right_output < -pwm_limit)
        speed_right_output = -pwm_limit;

    // ===================== 7. 更新历史误差 =====================
    speed_left_error_last_last = speed_left_error_last;
    speed_left_error_last = speed_left_error;

    speed_right_error_last_last = speed_right_error_last;
    speed_right_error_last = speed_right_error;

    // ===================== 8. 输出给电机 =====================
    Motor_pwm_set(speed_left_output, speed_right_output);
}


float Gyro_Feedforward = 0.0f; // 全局前馈输出量
int Kf_EVA_milli = 50;         // 前馈系数(千分比), 50 = 0.050

static int feedforward_avg_center(int start_row, int end_row)
{
    if (start_row < 0) start_row = 0;
    if (end_row > IMAGE_HEIGHT - 1) end_row = IMAGE_HEIGHT - 1;

    int sum = 0;
    int count = 0;
    for (int i = start_row; i <= end_row; i++)
    {
        sum += centralline[i];
        count++;
    }
    return (count > 0) ? (sum / count) : 0;
}

void Calculate_Obstacle_Feedforward(void)
{
    float target_feedforward = 0.0f;

    if (card_evade_phase == CARD_EVADE_MERGE) // 只在重新接回图像中线的并线阶段启用
    {
        // 多行平均替代单点取样,避免边界补线/夹边导致前馈跳变。
        int far_x = feedforward_avg_center(24, 32);
        int near_x = feedforward_avg_center(IMAGE_HEIGHT - 8, IMAGE_HEIGHT - 2);

        // 2. 计算近似曲率（远近点的像素差值）
        // 差值为正代表右转避障，差值为负代表左转避障
        float curvature = (float)(far_x - near_x);
        if (curvature > -2.0f && curvature < 2.0f)
        {
            curvature = 0.0f;
        }

        // 3. 核心前馈公式：前馈输出 = 系数 * 曲率 * 当前实际车速
        // 乘以 real_speed 是因为车速越快，通过相同曲率弯道所需的角速度和差速越大
        // Kf_EVA_milli 采用千分比整数, 方便菜单在线调参。
        float kf = static_cast<float>(Kf_EVA_milli) * 0.001f;
        target_feedforward = kf * curvature * real_speed;

        const float feedforward_limit = static_cast<float>(Gyro_limit) * 0.45f;
        if (target_feedforward > feedforward_limit) target_feedforward = feedforward_limit;
        if (target_feedforward < -feedforward_limit) target_feedforward = -feedforward_limit;
    }

    const float feedforward_alpha = (card_evade_phase == CARD_EVADE_MERGE) ? 0.25f : 0.15f;
    float next_feedforward = obstacle_feedforward_lpf +
                             (target_feedforward - obstacle_feedforward_lpf) * feedforward_alpha;

    const float feedforward_step_limit = 35.0f;
    float feedforward_step = next_feedforward - Gyro_Feedforward;
    if (feedforward_step > feedforward_step_limit)
    {
        next_feedforward = Gyro_Feedforward + feedforward_step_limit;
    }
    else if (feedforward_step < -feedforward_step_limit)
    {
        next_feedforward = Gyro_Feedforward - feedforward_step_limit;
    }

    obstacle_feedforward_lpf = next_feedforward;
    Gyro_Feedforward = next_feedforward;
    // printf("前馈==%f",Gyro_Feedforward);
}
//车向左转向时left_target下降，陀螺仪控制
// 建议在文件开头定义这三个避障专用参数（也可以放进 .hpp）
int Evade_Kp = 1;   // 避障直通 P：决定偏转力度（给大点，比如 30~60）
int Evade_KP2 =1;  //爆发力二次项
int Evade_Kd = 0;   // 避障直通 D：抵抗图像突变引起的车头抽搐
int ESP_D = 0;       // 车身稳定阻尼：防止车子差速过大导致原地甩尾打转
int pian_limit=100;//偏转限幅
int ED=0;
void Gyro_PID(void)    
{
    // 1. 陀螺仪滤波 (保持原样)
    gyro_speed_filtered = 0.8f * groyz_now + 0.2f * gyro_speed_filtered;
    //printf("%d %d\n",groyz_now_filtered,groyz_now);
    // groyz_now_filtered = groyz_now;
    float Dir_output_filtered = Dir_output;
    // Gyro_output = (int) Dir_output_filtered;
    // 2. 速度决策 (包含你原本的急刹车逻辑)
    speed_decision();

    // 卡片强制停车期间禁止方向环给左右轮叠加正负差速。
    // 否则 real_speed=0 时仍可能形成 (+Gyro_output,-Gyro_output) 原地旋转。
    if (card_stop_request == 1)
    {
        Gyro_output = 0;
        Gyro_error_last = 0;
        Gyro_Feedforward = 0.0f;
        speed_left_target = 0;
        speed_right_target = 0;
        return;
    }
    // 倒车阶段不读取图像方向误差：两轮给相同负速度，保证只直退、不在退车时甩头。
    // 到达目标距离后的制动阶段 real_speed=0，继续在这里强制撤掉全部方向差速。
    if (card_reverse_phase != CARD_REVERSE_IDLE)
    {
        Gyro_output = 0;
        Gyro_error_last = 0;
        Gyro_Feedforward = 0.0f;
        speed_left_target = real_speed;
        speed_right_target = real_speed;
        return;
    }
    Gyro_error = Dir_output_filtered - gyro_speed_filtered;
    // printf("*********%d\n",Gyro_error);
    Gyro_output = Gyro_kp * 0.01 * Gyro_error + Gyro_kd * 0.01 * (Gyro_error - Gyro_error_last);
    Calculate_Obstacle_Feedforward();
    if (card_evade_phase >= CARD_EVADE_TURN_IN &&
        card_evade_phase <= CARD_EVADE_SETTLE)
    {
        // evade_curve: 0=直道(EV_*Z) 1=弯道(EV_*W) 2=环岛(EV_*H)
       // printf("绕行角速度环");
        int gkp = (evade_curve == 2) ? Gyro_kp_EVA_H : (evade_curve == 1 ? Gyro_kp_EVA_W : Gyro_kp_EVA);
        int gkd = (evade_curve == 2) ? Gyro_kd_EVA_H : (evade_curve == 1 ? Gyro_kd_EVA_W : Gyro_kd_EVA);
        // TURN_IN必须有角速度闭环输出。某组场景参数尚未整定为0时回退到普通角速度P，
        // 避免识别完成后永远停在原地，等待3秒超时才继续。
        if (gkp <= 0) gkp = (Gyro_kp > 0) ? Gyro_kp : 1;
        Gyro_output = gkp * 0.01 * Gyro_error + gkd * 0.01 * (Gyro_error - Gyro_error_last);
    }

    // 角速度环输出一阶低通:平滑残余高频抖动(col_Differ量化等残留)。
    //   只滤PID部分,下面的前馈/硬保护/限幅都在滤波之后 → 前馈保持快响应、环岛硬保护仍是硬的。
    //   α = gyro_out_lpf×0.01: 100=不滤(默认,行为不变),越小越平滑但相位滞后越大(滤过头会画龙,别贪)。
    // {
    //     static float gyro_out_filt = 0.0f;
    //     float ga = gyro_out_lpf * 0.01f;
    //     if (ga < 0.05f) ga = 0.05f;   // 防止系数=0时输出卡死不动
    //     if (ga > 1.0f)  ga = 1.0f;
    //     gyro_out_filt = ga * Gyro_output + (1.0f - ga) * gyro_out_filt;
    //     Gyro_output = (int)gyro_out_filt;
    // }

    // 在反馈角速度环之后叠加绕行前馈,加快起手响应,但保留串级稳定性。
    if (card_evade_phase == CARD_EVADE_MERGE)
    {
        Gyro_output += static_cast<int>(Gyro_Feedforward);
    }

    // 环岛出环状态5方向硬保护:左环不允许右转差速,右环不允许左转差速。
    // if (left_halo_flag == 5 && Gyro_output > 0)  Gyro_output = 0;
    // if (right_halo_flag == 5 && Gyro_output < 0) Gyro_output = 0;

    if(Gyro_output > Gyro_limit) Gyro_output = Gyro_limit;
    if(Gyro_output < -1*Gyro_limit) Gyro_output = -1*Gyro_limit;    
    Gyro_error_last = Gyro_error;
    //------
    // Gyro_output = (int) Dir_output_filtered;
    //------
    speed_left_target  = real_speed + Gyro_output;
    speed_right_target = real_speed - Gyro_output;
   // printf("groypid\n");
}
void Dir_PID()     //方向环控制（外环）
{
    // TURN_IN/CLEAR 完全由相对航向角接管，不允许图像边线、环岛PID或旧补线抢控制权。
    if (card_evade_phase == CARD_EVADE_TURN_IN ||
        card_evade_phase == CARD_EVADE_CLEAR)
    {
        float angle_error = card_evade_target_yaw - card_evade_relative_yaw;
        float abs_error = (angle_error >= 0.0f) ? angle_error : -angle_error;
        int abs_gyro = (groyz_now >= 0) ? groyz_now : -groyz_now;
        float target_yaw_rate;

        if (card_evade_phase == CARD_EVADE_TURN_IN)
        {
            target_yaw_rate = angle_error * 25.0f;
            if (abs_error > CARD_EVADE_TURN_TOLERANCE)
            {
                if (target_yaw_rate > 0.0f && target_yaw_rate < 100.0f) target_yaw_rate = 100.0f;
                if (target_yaw_rate < 0.0f && target_yaw_rate > -100.0f) target_yaw_rate = -100.0f;
            }
            if (target_yaw_rate > 600.0f) target_yaw_rate = 600.0f;
            if (target_yaw_rate < -600.0f) target_yaw_rate = -600.0f;

            card_evade_turn_timeout_ticks++;
            if (abs_error <= CARD_EVADE_TURN_TOLERANCE && abs_gyro < 120)
                card_evade_turn_stable_ticks++;
            else
                card_evade_turn_stable_ticks = 0;

            if (card_evade_turn_stable_ticks >= CARD_EVADE_TURN_STABLE_TICKS ||
                card_evade_turn_timeout_ticks >= CARD_EVADE_TURN_TIMEOUT_TICKS)
            {
                bool timeout =
                    (card_evade_turn_timeout_ticks >= CARD_EVADE_TURN_TIMEOUT_TICKS);
                if (timeout)
                {
                    // 达不到设定角时保持当前已建立的角度，避免随后 CLEAR 继续原地追角。
                    card_evade_target_yaw = card_evade_relative_yaw;
                }
                card_evade_phase_dis = 0;
                card_evade_phase = CARD_EVADE_CLEAR;
                card_evade_turn_stable_ticks = 0;
                card_evade_turn_timeout_ticks = 0;
                Gyro_error_last = 0;
                printf("新绕行阶段: TURN_IN -> CLEAR (%s), yaw=%.2f target=%.2f\n",
                       timeout ? "timeout" : "stable",
                       static_cast<double>(card_evade_relative_yaw),
                       static_cast<double>(card_evade_target_yaw));
            }
        }
        else
        {
            // CLEAR 只做固定航向保持，速度由 speed_decision 设为 def_speed。
            target_yaw_rate = angle_error * 18.0f;
            if (target_yaw_rate > 300.0f) target_yaw_rate = 300.0f;
            if (target_yaw_rate < -300.0f) target_yaw_rate = -300.0f;
            card_evade_turn_stable_ticks = 0;
            card_evade_turn_timeout_ticks = 0;
        }

        Dir_output = static_cast<int>(target_yaw_rate);
        if (Dir_output > Dir_limit) Dir_output = Dir_limit;
        if (Dir_output < -Dir_limit) Dir_output = -Dir_limit;
        Last_col_Differ = col_Differ;
        return;
    }

    card_evade_turn_stable_ticks = 0;
    card_evade_turn_timeout_ticks = 0;
    //col_Differ=0;
    float gyro_help=0;
    gyro_help=fabs(groyz_now/100);
    if(gyro_help>5)gyro_help=5;
    //位置式pd计算方向环输出
    Dir_output=(KP_Fuzzy(col_Differ,col_Differ-Last_col_Differ,Dir_kp/2) + Dir_kp + gyro_help)*col_Differ+(Dir_kd+Kd_Fuzzy(col_Differ-Last_col_Differ,Dir_kd))*(col_Differ-Last_col_Differ);
    // Dir_output =  Dir_kp*0.1 * col_Differ +  Dir_kd *0.1* (col_Differ - Last_col_Differ);
    // 左环单独参数 (已修复重复相加)
    if(left_halo_flag != 0)
    {
        printf("左环方向环进入\n");
        Dir_output = (KP_Fuzzy(col_Differ, col_Differ - Last_col_Differ, Dir_Lkp_h/2) + Dir_Lkp_h + gyro_help) * col_Differ + (Dir_Lkd_h + Kd_Fuzzy(col_Differ - Last_col_Differ, Dir_Lkd_h)) * (col_Differ - Last_col_Differ);
        // Dir_output =  Dir_Lkp_h * col_Differ +  Dir_Lkd_h * (col_Differ - Last_col_Differ);
    }
    // 右环单独参数 (已修复重复相加)
    if(right_halo_flag != 0) 
    {

        Dir_output = (KP_Fuzzy(col_Differ, col_Differ - Last_col_Differ, Dir_Rkp_h/2) + Dir_Rkp_h + gyro_help) * col_Differ + (Dir_Rkd_h + Kd_Fuzzy(col_Differ - Last_col_Differ, Dir_Rkd_h)) * (col_Differ - Last_col_Differ);
        // Dir_output =  Dir_Rkp_h * col_Differ +  Dir_Rkd_h * (col_Differ - Last_col_Differ);
    }
    if (card_evade_phase == CARD_EVADE_MERGE ||
        card_evade_phase == CARD_EVADE_SETTLE)
    {
        // evade_curve: 0=直道(EV_*Z) 1=弯道(EV_*W) 2=环岛(EV_*H)
        //printf("绕行期");
        int dkp = (evade_curve == 2) ? Dir_kp_EVA_H : (evade_curve == 1 ? Dir_kp_EVA_W : Dir_kp_EVA);
        int dkd = (evade_curve == 2) ? Dir_kd_EVA_H : (evade_curve == 1 ? Dir_kd_EVA_W : Dir_kd_EVA);
        // 绕行期不用模糊控制,普通PD:输出 = kp*误差 + kd*误差变化(增益即菜单值本身,好整定)
        Dir_output = dkp * col_Differ + dkd * (col_Differ - Last_col_Differ);
    }

    // right_halo_flag == 5 && Dir_output < 0) Dir_output = 0;

    //双向限幅
    if(Dir_output>Dir_limit)Dir_output=Dir_limit;
    if(Dir_output<-1*Dir_limit)Dir_output=-1*Dir_limit;
    //更新误差记录
    Last_col_Differ=col_Differ;
   // printf("dirpid\n");
}
int brick_dis_isp;//红砖积分标志位
int brick_real_dis;//红砖实际行进距离积分
int brick_ju=0;//红砖积分距离判定(大于清零)
int Brick_handled=0;//上锁

// 红砖绕行与卡片预刹是两套互斥状态。红砖一旦正式启动，就撤销红砖验证阶段
// 可能留下的卡片预刹提示/积分，防止红砖绕行继续被 halS 压速或再次触发卡片停车。
static void clear_card_prebrake_for_brick()
{
    pre_brake = 0;
    pre_brake_hint = 0;
    pre_brake_giveup = 0;
    pre_brake_frames = 0;
    pre_brake_dis = 0;
    is_recognizing_now = 0;
    speed_left_error_integral = 0;
    speed_right_error_integral = 0;
}

void Distance_interial()
{
    // ==================== 0. 卡片识别后的倒车阶段 ====================
    // 冻结下面所有红砖/绕行/坡道距离状态，只累计车辆实际向后的平移距离。
    // 到距后立即进入制动阶段；制动滑行产生的实际后退距离仍继续累计，最终只补到
    // 新状态机的 CLEAR 清障距离中。
    if (card_reverse_phase != CARD_REVERSE_IDLE)
    {
        int reverse_speed_now = -speed_car;
        if (reverse_speed_now > 0)
        {
            distance_total_reverse += reverse_speed_now;
            while (distance_total_reverse >= 100)
            {
                card_reverse_real_dis += 1;
                distance_total_reverse -= 100;
            }
        }

        if (card_reverse_phase == CARD_REVERSE_DRIVING &&
            (card_reverse_target_dis <= 0 ||
             card_reverse_real_dis >= card_reverse_target_dis))
        {
            card_reverse_phase = CARD_REVERSE_BRAKING;
        }
        return;
    }
    distance_total_reverse = 0;
    
    // 💡 冷却距离阈值：代表避障完成后，车子需要再往前开多远才允许识别下一块砖
    // 你可以根据实车测试修改这个值（一般设为车身长度对应的值，比如 40 或 50）
    const int COOLDOWN_TARGET = 40; 

    // ==================== 1. 红砖积分处理 ====================
    // 触发条件：看到红砖 + 距离足够近 + 黑块纵坐标够低(近) + 当前没有在积分 + 💥当前不在冷却锁定期 + 不在绕行偏转中
    // (recog_staute==0：新卡片绕行期间冻结色块检测，也不允许红砖打断已锁定动作)
    // br_center_y = 黑块最下沿行(find_black_rect 里 custom_bottom_y)，>30 表示砖已足够近才启动距离积分。
    // ★禁区：整个环岛(halo≠0,进入/内部/出) / 十字内部(cross_cnt∈{1,2}) 不存在红砖 → 不启动红砖避障。
    int brick_forbid_zone =
        (left_halo_flag  != 0) ||
        (right_halo_flag != 0) ||
        (cross_cnt == 1 || cross_cnt == 2);
    if(brick_enable == 1 && Brick_flag == 1 && target_y2 > 20 && br_center_y > 30 && !brick_forbid_zone && brick_dis_isp == 0 && brick_cooldown_active == 0 && recog_staute == 0)
    {
        brick_dis_isp = 1; // 开始红砖距离积分
        // 必须在状态0->1的边沿立即撤销卡片预刹；本函数位于2ms中断中，下一次速度决策
        // 就会撤销 halS 覆盖，由现有正常/弯道速度决策接管，不必等到下一帧图像。
        clear_card_prebrake_for_brick();
        // 每块红砖只在此打印一次(brick_dis_isp 0->1 边沿;之后 find_black_rect 被门控关闭、
        // 标志冻结,直到绕行+冷却结束才可能再次触发下一块)
        // printf("检测到红砖: %s (中心x=%d)\n",
            //    (Brick_pos_flag == 1) ? "左砖->向右避" : "右砖->向左避", br_center_x);
    }

    if(brick_dis_isp == 1) // 红砖积分运行中
    {
        distance_total_brick += speed_car;
        
        // 修复余数丢失问题，保证高精度
        if(distance_total_brick >= 100)
        {
            brick_real_dis += 1;
            distance_total_brick -= 100; 
        }
        
        // 达到目标起偏距离，执行完成动作
        if(brick_real_dis >= brick_ju)
        {
            brick_dis_isp = 0;    // 关闭红砖积分
            brick_real_dis = 0;   // 积分清零
            distance_total_brick = 0;      
            
            // 🚀 核心修复：开启冷却锁！接下来的一段距离内，车子会对红砖“免疫”
            brick_cooldown_active = 1;  
        }
    }
    else if(brick_cooldown_active == 1) // 🚀 进入避障后的冷却期积分
    {
        // 用同样的车速积分原理，计算车子避障后往前开的距离
        brick_cooldown_total += speed_car;
        
        if(brick_cooldown_total >= 100)
        {
            brick_cooldown_real_dis += 1;
            brick_cooldown_total -= 100;
        }
        
        // 当车子盲开了足够的距离（彻底越过了这块砖）
        if(brick_cooldown_real_dis >= COOLDOWN_TARGET)
        {
            brick_cooldown_active = 0;      // 🔓 冷却结束，彻底解锁！
            brick_cooldown_real_dis = 0;    // 冷却距离清零
            brick_cooldown_total = 0;       // 余数清零
            
            // 此时系统已经准备好识别并处理赛道上的下一块红砖了！
        }
    }


    // ==================== 2. 新卡片绕行分阶段距离积分 ====================
    // TURN_IN原地转向不计距离；CLEAR/MERGE/SETTLE/STRAIGHT只累计正向平移量。
    // 每次阶段变化都丢弃上一阶段不足1个距离单位的余数，防止跨阶段串账。
    if (last_card_evade_distance_phase != card_evade_phase)
    {
        distance_total_pian = 0;
        last_card_evade_distance_phase = card_evade_phase;
    }

    if (card_evade_phase == CARD_EVADE_CLEAR ||
        card_evade_phase == CARD_EVADE_MERGE ||
        card_evade_phase == CARD_EVADE_SETTLE ||
        card_evade_phase == CARD_EVADE_STRAIGHT)
    {
        if (speed_car > 0) distance_total_pian += speed_car;
        while (distance_total_pian >= 100)
        {
            card_evade_phase_dis += 1;
            distance_total_pian -= 100;
        }

        if (card_evade_phase == CARD_EVADE_CLEAR)
        {
            int clear_base = (evade_clear_distance >= 0) ?
                             evade_clear_distance : -evade_clear_distance;
            int reverse_dis = (card_reverse_real_dis >= 0) ?
                              static_cast<int>(card_reverse_real_dis) :
                             -static_cast<int>(card_reverse_real_dis);
            int clear_target = clear_base + reverse_dis;
            if (card_evade_phase_dis >= clear_target)
            {
                card_evade_phase_dis = 0;
                distance_total_pian = 0;
                card_evade_phase = CARD_EVADE_MERGE;
                Gyro_error_last = 0;
                Gyro_Feedforward = 0.0f;
                printf("新绕行阶段: CLEAR -> MERGE, clear_target=%d, merge_target=%d\n",
                       clear_target, DETECT_PROCESS_DIS);
            }
        }
        else if (card_evade_phase == CARD_EVADE_MERGE)
        {
            int merge_target = (DETECT_PROCESS_DIS > 0) ? DETECT_PROCESS_DIS : 1;
            if (card_evade_phase_dis >= merge_target)
            {
                card_evade_phase_dis = 0;
                distance_total_pian = 0;
                card_evade_phase = CARD_EVADE_SETTLE;
                Gyro_Feedforward = 0.0f;
                printf("新绕行阶段: MERGE -> SETTLE, virtual offset=0\n");
            }
        }
    }

    // 绕行已经完全退出后才单独累计冷却路程。冷却只禁止重识别，不再改速度、前瞻和PID。
    if (card_evade_phase == CARD_EVADE_IDLE && evade_cooldown_active == 1)
    {
        if (speed_car > 0) distance_total_pian += speed_car;
        while (distance_total_pian >= 100)
        {
            Detect_real_dis += 1;
            distance_total_pian -= 100;
        }

        if (Detect_real_dis > EVADE_COOLDOWN_DIS)
        {
            evade_cooldown_active = 0;
            Detect_real_dis = 0;
            distance_total_pian = 0;
        }
    }


    // ===================== 3. 坡道积分处理 ===================
    if(slope_flag == 1 || slope_flag == 2)
    {
        distance_total_slope += speed_car;
        
        // 修复余数丢失问题
        if(distance_total_slope >= 100)
        {
            slope_process_dis += 1;
            distance_total_slope -= 100;
        }
        
        if(slope_process_dis > slope_ju)
        {
            slope_flag = 2;
        }
        
        if(slope_process_dis >= 300)
        {
            slope_flag = 0;
            slope_process_dis = 0;
            distance_total_slope = 0;
        }
    }

    // ==================== 4. 识别前刹车安全兜底：起刹后距离积分 ====================
    // 起刹(pre_brake==1)后累加行进距离;走满 PRE_BRAKE_MAX_DIS 仍没识别(recog_staute 仍为0)→
    // 置放弃标志 pre_brake_giveup(由 update_pre_brake 退出减速、恢复正常循迹),防止误检或
    // 目标始终够不到 DETECT_START_DIS 时一直卡在刹车状态。pre_brake==0 时积分清零。
    if(pre_brake == 1)
    {
        distance_total_prebrake += speed_car;
        if(distance_total_prebrake >= 100)
        {
            pre_brake_dis += 1;
            distance_total_prebrake -= 100;
        }
        if(pre_brake_dis > PRE_BRAKE_MAX_DIS) pre_brake_giveup = 1;  // 超距离仍没识别 → 放弃(pre_brake 由主循环清)
    }
    else
    {
        pre_brake_dis = 0;
        distance_total_prebrake = 0;
    }
   // printf("distance\n");
}
int halt_time=0;//刹车时间
int halt_sp=0;//刹车速度
// 卡片"减速 + 负压增强"统一锁：卡片预刹/分类绕行/回正期间置1；
// 由 speed_decision 维护并做 40Hz 防抖。减速(本函数)与负压(isr 风机)共用此标志，
// 保证"切负压"和"减速"在检测到红块的同一时刻一起触发、一起恢复，不必等推理完成。
int evade_slow_lock = 0;

// 识别前期刹车 latch 维护——在主循环每检测帧调用一次(见 main，紧跟 Detect_process 之后)。
//   红块确认(Target_flag/is_recognizing)一次即起刹;起刹后在 target_y2 到达 DETECT_START_DIS
//   (即识别发生 recog_staute=1)之前【不退出】刹车——即使黑块暂时丢失也保持(速度冻结,见 taper)。
//   安全兜底:起刹后行进距离积分(Distance_interial)超过 PRE_BRAKE_MAX_DIS 仍没识别 →
//   pre_brake_giveup=1,这里据此强制退出减速、恢复正常循迹,并锁到目标真正过去才允许再起刹。
void update_pre_brake(void)
{
    //return;
    // 卡片识别总开关关闭时，禁止黑块候选触发卡片预刹、停车和停稳锁。
    // 红砖的 Brick_flag/brick_dis_isp 独立处理，不在这里清除。
    if (card_recognition_disable == 1)
    {
        pre_brake = 0;
        pre_brake_hint = 0;
        pre_brake_giveup = 0;
        pre_brake_frames = 0;
        pre_brake_dis = 0;
        card_stop_request = 0;
        card_vehicle_stopped = 0;
        is_recognizing_now = 0;
        Target_flag = 0;
        return;
    }

    // 红砖已经进入正式绕行：持续禁止卡片预刹重新上锁。
    // pre_brake_hint 可能在红砖完整分类前由红底早检产生，因此必须放在所有起刹条件之前。
    if (brick_dis_isp == 1)
    {
        clear_card_prebrake_for_brick();
        return;
    }

    // 有效卡片刚完成分类后的时间冷却只屏蔽“下一张卡片”的识别链。
    // 当前卡片已经进入recog_staute=1的倒车/绕行时不能在这里清状态，否则会打断现有动作。
    if (card_recognition_cooldown_ticks > 0 &&
        recog_staute == 0 && card_stop_request == 0)
    {
        pre_brake = 0;
        pre_brake_hint = 0;
        pre_brake_giveup = 0;
        pre_brake_frames = 0;
        pre_brake_dis = 0;
        is_recognizing_now = 0;
        return;
    }

    int halo_no_black_check =
        ((left_halo_flag  != 0 && left_halo_flag  != 4) ||
         (right_halo_flag != 0 && right_halo_flag != 4));

    if (halo_no_black_check)
    {
        Target_flag = 0;
        target_y2 = 0;
        is_recognizing_now = 0;
        pre_brake = 0;
        pre_brake_giveup = 0;
        pre_brake_frames = 0;
        speed_left_error_integral = 0;
        speed_right_error_integral = 0;
        return;
    }
    if (recog_staute == 1)                                { pre_brake = 0; pre_brake_giveup = 0; }  // 已识别→正常退出,复位
    else if (pre_brake_giveup == 1)                                                                 // 兜底放弃→退出减速,恢复正常循迹
    {
        // [排查] 只在真正松刹的那一帧打印:走满 PRE_BRAKE_MAX_DIS 还没识别 → 这次起刹多半是误检
        if (pre_brake == 1)
            printf("刹车放弃(走满%d没识别→误检)! y2=%d\n", PRE_BRAKE_MAX_DIS, target_y2);
        pre_brake = 0;
    }
    // 红块确认(必须过【完整验证】:粗筛+位置约束+提取R2+底座R3 → Target_flag=1)且未放弃→起刹/维持。
    // 已去掉"远距提示(finder级,跳过R2/R3)"通道:它虽然能提前到 y2≈19~24 起刹,但门槛太松,
    // 弯道/环岛里的假黑块+反光红会误锁 pre_brake 造成随意刹车。
    else if (Target_flag == 1 || is_recognizing_now == 1 || (pre_brake_hint == 1 && evade_cooldown_active == 0))
    {
        if (pre_brake == 0)   // ★起刹 0→1 边沿(每块红块必然且仅一次)：积分打负=反向刹车偏置
        {
            // 这里保留轻量反向预置，只负责"提醒控制器进入减速态"；
            // 真正的减速主力交给上面的主动刹车 + 速度斜坡，避免一下子点头。
            speed_left_error_integral  = -4000;
            speed_right_error_integral = -4000;
            // [排查] 起刹现场:sp=当时车速 y2=目标距离 EvaSD=识别触发距离(窗口=EvaSD-y2)
            //        场景: wd=弯道(0直/±1弯) halo=环岛(左/右状态,0=不在) cross=十字
            //        → 对着这行就能看出每次刹车发生在赛道的哪种场景
            printf("起刹! sp=%d y2=%d EvaSD=%d | wd=%d halo=%d/%d cross=%d\n",
                   speed_car, target_y2, DETECT_START_DIS,
                   wd_flag_stable, left_halo_flag, right_halo_flag, cross_cnt);
        }
        pre_brake = 1;
        printf("brake\n");
    }
    // 否则(起刹后、未识别、黑块暂丢、未放弃)：pre_brake 保持不变 → 抗黑块短暂闪断,识别前不退出。
    // 误检兜底只靠距离:起刹后走满 PRE_BRAKE_MAX_DIS 仍没识别 → pre_brake_giveup=1(见 Distance_interial),
    // 由上面第二个分支松刹。红色完整验证已足够准,不再做帧数级的"目标消失"误检判断。

    // 放弃锁复位:目标真正过去了(黑块也没了)才允许下次再起刹,避免同一目标反复刹
    if (pre_brake_giveup == 1 && target_y2 <= 0 && Target_flag == 0) pre_brake_giveup = 0;

    // 已刹车帧数计数:pre_brake==1期间累加，松开时清零。本函数在Detect_process之前调用。
    // 现在该计数只参与“正式NCNN输入条件”判定；满足条件后仍必须经过独立的编码器停稳确认，
    // 不会再在起刹同一图像帧直接识别和偏转。
    if (pre_brake == 1) pre_brake_frames++;
    else                pre_brake_frames = 0;
}
void speed_decision()
{
    int target_speed = base_speed;
    int halo_force_curve_speed =
        ((left_halo_flag  != 0 && left_halo_flag  != 4) ||
         (right_halo_flag != 0 && right_halo_flag != 4));

    // 弯道 / 环岛 / 十字基础速度：稳定弯道(wd_flag_stable=±1) 或 处于环岛任一状态(halo_flag=1~6)
    // 或 处于十字(cross_cnt=1补线中/2退出段)时，统一用单独的弯道速度 curve_speed。
    // 优先级低于坡道/避障(下面若命中会再覆盖)。
    if(wd_flag_stable == -1 || wd_flag_stable == 1
       || left_halo_flag != 0 || right_halo_flag != 0
       || cross_cnt != 0)
    {
        target_speed = curve_speed;
    }

    // 坡道速度判断 (优先级低于避障)
    if(slope_flag == 1)
    {
        target_speed = slope_speed;
        printf("坡道上坡\n");
    }

    // 卡片已经满足正式NCNN输入条件：立即将速度指令清零。
    // ISR同时直接调用 speed_close_PID(0,0)，这里清零速度指令滤波是为了分类完成后从0平滑起步。
    if (card_stop_request == 1)
    {
        evade_slow_lock = 1;
        speed_command_lpf = 0.0f;
        real_speed = 0;
        return;
    }

    // 分类结果已经锁存后的独立倒车阶段。这里使用负的平移速度；Gyro_PID 会
    // 强制左右轮相同，禁止图像方向环在倒车时叠加差速。
    if (card_reverse_phase == CARD_REVERSE_DRIVING)
    {
        int reverse_speed = (card_reverse_speed >= 0) ? card_reverse_speed : -card_reverse_speed;
        evade_slow_lock = 1;
        speed_command_lpf += (static_cast<float>(-reverse_speed) - speed_command_lpf) * 0.20f;
        real_speed = static_cast<int>(speed_command_lpf);
        return;
    }
    if (card_reverse_phase == CARD_REVERSE_BRAKING)
    {
        evade_slow_lock = 1;
        speed_command_lpf = 0.0f;
        real_speed = 0;
        return;
    }

    // 新绕行执行优先级高于环岛/坡道/旧视觉减速。
    if (card_evade_phase == CARD_EVADE_TURN_IN)
    {
        // 平移速度为0，但Gyro_PID仍会叠加正负差速，允许原地建立固定角度。
        evade_slow_lock = 1;
        speed_command_lpf = 0.0f;
        real_speed = 0;
        return;
    }
    if (card_evade_phase == CARD_EVADE_CLEAR ||
        card_evade_phase == CARD_EVADE_MERGE ||
        card_evade_phase == CARD_EVADE_SETTLE)
    {
        evade_slow_lock = 1;
        int maneuver_speed = (def_speed >= 0) ? def_speed : -def_speed;
        if (speed_command_lpf < 0.0f) speed_command_lpf = 0.0f;
        speed_command_lpf +=
            (static_cast<float>(maneuver_speed) - speed_command_lpf) * 0.12f;
        real_speed = static_cast<int>(speed_command_lpf);
        return;
    }

    // 环岛非4状态不检测色块,速度也固定为弯道速度,防止残留预刹/绕行锁覆盖成 halt_sp/def_speed。
    if (halo_force_curve_speed)
    {
        evade_slow_lock = 0;
        target_speed = curve_speed;
        if (speed_command_lpf <= 0.0f) speed_command_lpf = static_cast<float>(target_speed);
        float alpha = (target_speed < speed_command_lpf) ? 0.35f : 0.12f;
        speed_command_lpf += (static_cast<float>(target_speed) - speed_command_lpf) * alpha;
        real_speed = static_cast<int>(speed_command_lpf);
        return;
    }

    // 黑块软限速必须保留到本轮所有预刹速度决策结束后再应用。有效卡片通常在同一图像帧
    // 就把 pre_brake 置1；若在这里直接改 target_speed，后面的预刹混合速度会再次覆盖它。
    int black_soft_limit_active =
        (card_recognition_disable == 0) &&
        ((br_found == 1) || (pre_brake == 1)) &&
        (recog_staute == 0) &&
        (evade_cooldown_active == 0) &&
        (brick_dis_isp == 0) &&
        (black_soft_speed > 0);

    // ==========================================================
    // 避障减速：识别前期"起刹靠红块确认、保持靠二值黑块"，不用掉帧计时锁。
    //   pre_brake 由主循环 update_pre_brake() 按"检测帧"维护(红块确认起刹 → 黑块 target_y2>0
    //   保持 → 黑块连续消失极短兜底帧数后才松)。这里直接读取。
    // ==========================================================

    // 【1. 提取所有需要减速的视觉信号】
    int need_slow = 0;
    if (pre_brake == 1)                        need_slow = 1; // 识别前期(黑块保持,抗红块闪断)

    // APC(类别0)/ambulance(类别2) → 车辆(turn_action==1)：识别确认后当作正常循迹，
    // 立刻解除减速锁 → 速度回正常(base/curve)、走正常速度环 PID(非软PID)，
    // 直行通过期与其后的识别冷却期全程不减速。
    if ((final_detect == 0 || final_detect == 2) && recog_staute == 1)
        need_slow = 0;

    // 【2. 减速锁 = need_slow】积分清零已统一挪到 update_pre_brake 的 pre_brake 0→1 边沿
    // (每块红块必然且仅清一次)，此处不再按 need_slow 边沿清——need_slow 在直行/状态切换时
    // 会中途掉0再回1,产生多余边沿导致重复清零。
    evade_slow_lock = need_slow;

    // ==========================================================
    // 这里只处理识别前预刹；TURN_IN/CLEAR/MERGE/SETTLE已在上方按阶段提前返回。
    // ==========================================================
    if (evade_slow_lock == 1)
    {
        if (recog_staute == 1 && turn_action == 1)
        {
            target_speed = base_speed;   // 直行车辆：推理完成即恢复base_speed直冲过去(照样吃绕行的积分距离判定退出)
        }
        else
        {
            // 识别前期：按目标靠近程度从当前正常速度平滑过渡到 halt_sp。
            // 正式状态机启动后才在上方切到对应速度，避免预刹速度被提前抬高。
            int near_speed = halt_sp;

            int far_speed = target_speed;
            int start_dis = DETECT_START_DIS;
            int y2 = target_y2;

            if (start_dis > 5 && y2 > 0)
            {
                if (y2 >= start_dis)
                {
                    target_speed = near_speed;
                }
                else
                {
                    // ratio: 0=刚看到目标, 1=逼近识别触发点
                    float ratio = static_cast<float>(y2) / static_cast<float>(start_dis);
                    if (ratio < 0.0f) ratio = 0.0f;
                    if (ratio > 1.0f) ratio = 1.0f;

                    // 前半段减速轻一点，后半段再明显收速，避免一开始就顿。
                    float blend = ratio * ratio;
                    float mixed_speed = static_cast<float>(far_speed) +
                                        (static_cast<float>(near_speed - far_speed) * blend);
                    target_speed = static_cast<int>(mixed_speed);
                }
            }
            else
            {
                target_speed = near_speed;
            }
        }

    }

    // 最终速度上限：黑块候选出现，或由该目标锁存的pre_brake仍在持续时，预刹混合
    // 结果都不能高于 BkSp。这样即使黑块短暂丢帧，BkSp=40也会真实反映到real_speed。
    if (black_soft_limit_active && target_speed > black_soft_speed)
    {
        target_speed = black_soft_speed;
    }

    // 最后再做一层一阶平滑。减速时更跟手，回升时更柔和。
    if (speed_command_lpf <= 0.0f) speed_command_lpf = static_cast<float>(target_speed);

    float alpha = (target_speed < speed_command_lpf) ? 0.35f : 0.12f;
    speed_command_lpf += (static_cast<float>(target_speed) - speed_command_lpf) * alpha;
    real_speed = static_cast<int>(speed_command_lpf);
}
void turn_propect_decision()//打角行决策函数
{
    // 正常循迹
    turn_propect = base_turn_propect;

    //环岛进入状态
    if(left_halo_flag == 1 || left_halo_flag ==2 || left_halo_flag ==3 || right_halo_flag == 1 || right_halo_flag ==2 || right_halo_flag ==3)
    {
        turn_propect = base_turn_propect+5;
    }
    // 只有MERGE/SETTLE重新读取图像中线，才切换到绕行前瞻；TURN_IN/CLEAR由角度闭环接管，
    // 冷却期已经回到正常循迹，不再沿用绕行前瞻。
    if (card_evade_phase == CARD_EVADE_MERGE ||
        card_evade_phase == CARD_EVADE_SETTLE)
    {
        turn_propect = (evade_curve == 2) ? halo_turn_propect : cd_turn_propect;
    }
    //限幅，防止数组越界
    if(turn_propect<End_Line)turn_propect=End_Line;
    // 上限夹到 IMAGE_HEIGHT-3(=57)：getdiffer 会读前瞻行往下连续3行(forward_row+2)，
    // 预留2行余量，避免 centralline[] 越界。
    if(turn_propect>=IMAGE_HEIGHT-3)turn_propect=IMAGE_HEIGHT-3;
}
//陀螺仪积分
int angle_Y=0;//Y轴角度
float Evade_angle_z=0;
void gyro_integrate()
{
    if (card_evade_phase == CARD_EVADE_TURN_IN ||
        card_evade_phase == CARD_EVADE_CLEAR)
    {
        card_evade_relative_yaw +=
            static_cast<float>(groyz_now) * CARD_EVADE_YAW_DT;
    }

    if(enable_gyro_flag)//Z轴积分,
    {
        float dt=0.0002;
        gyro_angle_z_add+=groyz_now*dt;//gyro_angle_z_rate*dt;//+gyro_angle_z_acc_add
        if(gyro_angle_z_add>180)
            gyro_angle_z_add=0;
        if(gyro_angle_z_add<-180)
            gyro_angle_z_add=0;
    }
    // if (recog_staute == 1 && turn_action != 1) //偏转角度判定，超出阈值直接退出
    // {
    //     float dt=0.0002;
    //     Evade_angle_z+=groyz_now*dt;
    //     if(Evade_angle_z>180)Evade_angle_z=0;
    //     if(Evade_angle_z<-180)Evade_angle_z=0;
    //     if(abs(Evade_angle_z)>8)
    //     {
    //         recog_staute=0;//超出阈值退出偏转
    //         Evade_angle_z=0;//积分值清零

    //         // 👇 必须加上这三行！
    //         Detect_dis_isp = 0;  
    //         Detect_real_dis = 0; 
    //         recog_cnt = 0;
    //     }
    // }
    // Y轴积分，判断坡道
    {
        float dt=0.0002;
        angle_Y+=groyy_now*dt;
        // printf("Y轴角度=%d\n",angle_Y);
    }
    //printf("gyroz,int\n");

}
void Get_DIS()
{
    TOF_DIS=163;//如果未测到或者不测了就用163表示
    if(slope_flag==0)
    {
        TOF_DIS=TOF.get_distance()/50;//测距
    }
    if(TOF_DIS > 0 && TOF_DIS <= TOF_ju) // 假设有效距离大于0//表示判到坡道元素
    {
        slope_flag=1;
    }
}

void reset_control_runtime_state(void)
{
    // 事件计数、斑马线延时、卡片序号与时间冷却。
    reset_run_stop_counters();

    // 卡片预刹、停车、倒车与绕行控制状态。
    pre_brake = 0;
    pre_brake_hint = 0;
    pre_brake_giveup = 0;
    pre_brake_dis = 0;
    pre_brake_frames = 0;
    card_stop_request = 0;
    card_vehicle_stopped = 0;
    card_reverse_phase = CARD_REVERSE_IDLE;
    card_reverse_stopped = 0;
    card_reverse_real_dis = 0;
    card_stop_stable_ticks = 0;
    card_reverse_stable_ticks = 0;
    card_recognition_cooldown_ticks = 0;
    evade_slow_lock = 0;
    evade_curve = 0;
    evade_reverse = 0;
    brake_debug_count = 0;
    halt_time = 0;
    ED = 0;

    // 红砖、坡道及全部距离积分余数/冷却锁。
    brick_dis_isp = 0;
    brick_real_dis = 0;
    Brick_handled = 0;
    brick_cooldown_active = 0;
    brick_cooldown_total = 0;
    brick_cooldown_real_dis = 0;
    distance_total_brick = 0;
    distance_total_pian = 0;
    distance_total_reverse = 0;
    distance_total_prebrake = 0;
    distance_total_slope = 0;
    last_card_evade_distance_phase = CARD_EVADE_IDLE;
    card_evade_turn_stable_ticks = 0;
    card_evade_turn_timeout_ticks = 0;
    slope_flag = 0;
    slope_process_dis = 0;
    TOF_DIS = 0;

    // 速度测量、速度环和输出历史。
    memset(filter_spL, 0, sizeof(filter_spL));
    memset(filter_spR, 0, sizeof(filter_spR));
    speed_left_now = 0;
    speed_right_now = 0;
    speed_car = 0;
    speed_left_error = 0;
    speed_right_error = 0;
    speed_left_error_integral = 0;
    speed_right_error_integral = 0;
    speed_left_error_last = 0;
    speed_right_error_last = 0;
    speed_left_error_last_last = 0;
    speed_right_error_last_last = 0;
    speed_left_output = 0;
    speed_right_output = 0;
    speed_left_target = 0;
    speed_right_target = 0;
    L_speedmotor = 0;
    R_speedmotor = 0;
    real_speed = 0;
    speed_command_lpf = 0.0f;

    // 方向环、角速度环、前馈与角度积分历史。
    Dir_output = 0;
    Gyro_error = 0;
    Gyro_error_integral = 0;
    Gyro_error_last = 0;
    Gyro_output = 0;
    Gyro_Feedforward = 0.0f;
    obstacle_feedforward_lpf = 0.0f;
    gyro_speed_filtered = 0.0f;
    gyro_kalman_initialized = 0;
    gyro_kalman_estimate = 0.0f;
    gyro_kalman_error_cov = 1.0f;
    groyz_now = 0;
    groyy_now = 0;
    angle_Y = 0;
    Evade_angle_z = 0.0f;
    Last_col_Differ = col_Differ;
    start_flag = 0;
}
