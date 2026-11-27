#include "zf_common_headfile.hpp"
// ===================== 全局变量 =====================
uint8_t car_running = 0;
std::atomic<uint8_t> launch_request(0);
uint8_t screen_clear_flag = 0;

int menu_temp_index = 0;
int menu_page_num   = PAGE_HOME;
int menu_last_row   = 0;
int im_dis_type=0;//显示图像类型，0---灰度图像，1---二值化图像
int camera_exposure_mode = 1;    //0=自动曝光，1=手动曝光
int camera_manual_exposure = 140;//手动曝光默认值；相机菜单每次±5
int inte=1234;
// ===================== 实时显示参数 =====================
DisplayItem_int real_time_int[] = {
    {"groyz",  &groyz_now},
    {"SPL",   &speed_left_now},
    {"SPR",   &speed_right_now},
    {"DIRO",   &Dir_output},
    {"GYRO",  &Gyro_output},
    {"TGTL",  &speed_left_target},
    {"TGTR",  &speed_right_target},
    {"ReaS",&real_speed},
    {"TP",&turn_propect},
    // {"WF",&wd_flag},
    // {"EC",&evade_curve},//绕行参数选择:0=直道参数(EV_*Z),1=弯道参数(EV_*W)
    // {"BNF",& bottom_narrow_flag},
    // {"EL",&evade_slow_lock},

    // {"EvaS",&DETECT_START_DIS},
    // {"cs",&curve_score },
    // {"cd",&curve_dir},
    // {"min",&min_efect_row_end},
    {"li",&left_halo_flag},
    {"ri",&right_halo_flag},
    // {"LSI",&speed_left_error_integral},
    // {"RSI",&speed_right_error_integral},
};
const int real_time_int_LENGTH = sizeof(real_time_int) / sizeof(DisplayItem_int);

DisplayItem_float real_time_float[] = {
    {"COL", &col_Differ},
    {"GFF", &Gyro_Feedforward},
    // {"GZO",&gyro_angle_z_add},
    // {"EZO",&Evade_angle_z},
    // {"QK",&Gyro_Feedforward},
};
const int real_time_float_LENGTH = sizeof(real_time_float) / sizeof(DisplayItem_float);

// ===================== 5页参数 =====================
int p1_kp=20, p1_ki=1, p1_kd=5, p1_th=120;
int p2_spd=300, p2_acc=50;
int p3_srv=1500, p3_trim=0;
int p4_max=800;
int p5_filt=2, p5_turn=20;
int cam_th=110, cam_bin=1;
//每个结构体最多放置14个参数
DisplayItem_int Para1[] = {
    {"Basp",&base_speed},
    {"Pisp",&def_speed},
    {"BasTp",&base_turn_propect},
    {"CDTp",&cd_turn_propect},//绕行退出冷却期的前瞻行(回正/接弯道)
    // {"PiTp",&def_turn_propect},
    // {"De_S_DIS",&DETECT_START_DIS},
    // {"De_P_DIS",&DETECT_PROCESS_DIS},
    {"HLKP",&Dir_Lkp_h},
    {"HLKD",&Dir_Lkd_h},
    {"HRKP",&Dir_Rkp_h},
    {"HRKD",&Dir_Rkd_h},
    {"Brick_D",&brick_ju},
    {"Slope_D",&slope_ju},
    {"TOF_D",&TOF_ju},
    {"FAN",&FAN_SET},
    {"Wdsp",&curve_speed},//弯道基础速度
    {"halS",&halt_sp},//卡片识别前期刹车速度(红砖正式绕行后不再使用)

};
const int Para1_LENGTH = sizeof(Para1) / sizeof(Para1[0]);

DisplayItem_int Para2[] = {
    {"Kpl",&kpL_speed_close},
    {"Kil",&kiL_speed_close},
    {"Kdl",&kdL_speed_close},
    {"Kpr",&kpR_speed_close},
    {"Kir",&kiR_speed_close},
    {"Kdr",&kdR_speed_close},
    {"DirP",&Dir_kp},
    {"DirD",&Dir_kd},
    // {"DirSWP",&Dir_SWP},
    // {"DirSWD",&Dir_SWD},
    {"GyrP",& Gyro_kp},
    {"GyrD",& Gyro_kd},
    // {"GOLpf",&gyro_out_lpf},//角速度环输出低通(×0.01=α,100=不滤,越小越平滑越滞后)
    {"BrkEn",&brick_enable},//红砖识别总开关:0=关,1=开

};
const int Para2_LENGTH = sizeof(Para2) / sizeof(Para2[0]);

DisplayItem_int Para3[] = {
    {"EV_DkpZ",&Dir_kp_EVA},//直道绕行方向环P
    {"EV_DkdZ",&Dir_kd_EVA},//直道绕行方向环D
    // {"halT",&halt_time},
    {"EDIPSN",&EvaD_N},
    {"EDIPSS",&EvaD_S},
    {"EDIPSO",&EvaD_O},
    {"ESDW",&EvaSD_W},
    {"ESDZ",&EvaSD_Z},
    {"ESDH",& EvaSD_H},
    {"EV_GkpZ",&Gyro_kp_EVA},//直道绕行角速度环P
    {"EV_GkdZ",&Gyro_kd_EVA},//直道绕行角速度环D
    {"DefAct",&fuzzy_default_action},//置信度<0.6模糊时默认动作:0左拐/1直行/2右拐
    {"EvOft",&EVADE_OFFSET},//并线阶段初始虚拟中线偏移像素
    {"EvAng",&evade_turn_angle},//倒车停稳后的固定偏转角绝对值
    {"EvClr",&evade_clear_distance},//固定角度后向前清障距离；实际值还会加实际倒车距离
};
const int Para3_LENGTH = sizeof(Para3) / sizeof(Para3[0]);

DisplayItem_int Para4[] = {
    {"EvaSkp",&Skp},
    {"EvaSki",&Ski},
    {"EvaSkd",&Skd},
    {"BrkF",&brake_force},//起刹主动刹车力度(越大刹越猛)
    {"EV_DkpW",&Dir_kp_EVA_W},//弯道绕行方向环P
    {"EV_DkdW",&Dir_kd_EVA_W},//弯道绕行方向环D
    {"EV_GkpW",&Gyro_kp_EVA_W},//弯道绕行角速度环P
    {"EV_GkdW",&Gyro_kd_EVA_W},//弯道绕行角速度环D
    {"EV_DkpH",&Dir_kp_EVA_H},//环岛绕行方向环P
    {"EV_DkdH",&Dir_kd_EVA_H},//环岛绕行方向环D
    {"EV_GkpH",&Gyro_kp_EVA_H},//环岛绕行角速度环P
    {"EV_GkdH",&Gyro_kd_EVA_H},//环岛绕行角速度环D
    {"EV_TpH",&halo_turn_propect},//环岛绕行前瞻行(其余绕行用Para1的CDTp)
    {"EV_Kf",&Kf_EVA_milli},//绕行角速度前馈系数(千分比,50=0.050)
};
const int Para4_LENGTH = sizeof(Para4) / sizeof(Para4[0]);

DisplayItem_int Para5[] = {
    {"P5-Flt", &p5_filt},
    {"P5-Trn", &p5_turn},
    {"RvDis", &card_reverse_target_dis},//卡片分类完成后的倒车距离，0=关闭
    {"RvSp",  &card_reverse_speed},     //倒车速度绝对值
    {"BkSp",  &black_soft_speed},       //黑块候选一级软限速；parameter_new.txt第61行
    {"ZbStop",&zebra_stop_target_count},//n对应跑n+1圈；负数关闭
    {"CdStop",&card_stop_target_count}, //第m个完成NCNN分类的卡片停车；0=关闭
    {"CdCool",&card_recognition_cooldown_100ms},//卡片有效分类后的识别冷却；单位100ms，10=1秒，0=关闭
    {"CdOff",&card_recognition_disable},//卡片识别总开关：0=正常识别，1=关闭赛道卡片识别
    {"Card1",&card_sequence_enable[0]},//第1张卡片：0=不识别直接通过，1=正常识别
    {"Card2",&card_sequence_enable[1]},//第2张卡片：0=不识别直接通过，1=正常识别
    {"Card3",&card_sequence_enable[2]},//第3张卡片：0=不识别直接通过，1=正常识别
    {"Card4",&card_sequence_enable[3]},//第4张卡片：0=不识别直接通过，1=正常识别
    {"Card5",&card_sequence_enable[4]},//第5张卡片：0=不识别直接通过，1=正常识别
};
const int Para5_LENGTH = sizeof(Para5) / sizeof(Para5[0]);

DisplayItem_int Cama1[] = {
    {"ExpMode",&camera_exposure_mode},
    {"ManExpo",&camera_manual_exposure},
    {"IMDT",&im_dis_type},
    {"CAM-Th", &cam_th},
    {"CAM-Bl", &cam_bin},
    {"CAM-B2", &cam_bin},
};
const int Cama1_LENGTH = sizeof(Cama1) / sizeof(Cama1[0]);

// ===================== 画矩形 =====================
void draw_rect_box(int x1, int y1, int x2, int y2, uint16 color)
{
    ips200.draw_line(x1, y1, x2, y1, color);
    ips200.draw_line(x1, y2, x2, y2, color);
    ips200.draw_line(x1, y1, x1, y2, color);
    ips200.draw_line(x2, y1, x2, y2, color);
}

// ===================== 绘制下划线 =====================
void menu_draw_underline(int y_pos, uint16 color)
{
    if(menu_page_num == PAGE_HOME)
    {
        // 右上角正确划线
        ips200.draw_line(IMAGE_WIDTH+20, y_pos + FONT_HEIGHT - 2, IMAGE_WIDTH+80, y_pos + FONT_HEIGHT - 2, color);
    }
    else
    {
        ips200.draw_line(10, y_pos + FONT_HEIGHT - 2, 230, y_pos + FONT_HEIGHT - 2, color);
    }
}

//**************************************************************************
// 首页右上角菜单
//**************************************************************************
void menu_draw_home(void)
{
    ips200.show_string(IMAGE_WIDTH+20,  0, "START");
    ips200.show_string(IMAGE_WIDTH+20, 20, "PARA");
    ips200.show_string(IMAGE_WIDTH+20, 40, "CAMA");

    menu_draw_underline(menu_last_row * 20, RGB565_WHITE);
    menu_draw_underline(menu_temp_index * 20, RGB565_RED);

    menu_last_row = menu_temp_index;
}

// ===================== 参数页 =====================
void menu_draw_para1(void)
{
    ips200.clear();
    ips200.show_string(0,0,"Para 1/5");
    for(int i=0;i<Para1_LENGTH;i++){
        ips200.show_string(10,(i+1)*FONT_HEIGHT,Para1[i].label);
        ips200.show_int(120,(i+1)*FONT_HEIGHT,*Para1[i].value,4);
    }
    menu_draw_underline(menu_last_row*FONT_HEIGHT,RGB565_WHITE);
    menu_draw_underline(menu_temp_index*FONT_HEIGHT,RGB565_RED);
    menu_last_row=menu_temp_index;
}

void menu_draw_para2(void){
    ips200.clear(); ips200.show_string(0,0,"Para 2/5");
    for(int i=0;i<Para2_LENGTH;i++){ips200.show_string(10,(i+1)*FONT_HEIGHT,Para2[i].label); ips200.show_int(120,(i+1)*FONT_HEIGHT,*Para2[i].value,4);}
    menu_draw_underline(menu_last_row*FONT_HEIGHT,RGB565_WHITE); menu_draw_underline(menu_temp_index*FONT_HEIGHT,RGB565_RED); menu_last_row=menu_temp_index;
}

void menu_draw_para3(void){
    ips200.clear(); ips200.show_string(0,0,"Para 3/5");
    for(int i=0;i<Para3_LENGTH;i++){ips200.show_string(10,(i+1)*FONT_HEIGHT,Para3[i].label); ips200.show_int(120,(i+1)*FONT_HEIGHT,*Para3[i].value,4);}
    menu_draw_underline(menu_last_row*FONT_HEIGHT,RGB565_WHITE); menu_draw_underline(menu_temp_index*FONT_HEIGHT,RGB565_RED); menu_last_row=menu_temp_index;
}

void menu_draw_para4(void){
    ips200.clear(); ips200.show_string(0,0,"Para 4/5");
    for(int i=0;i<Para4_LENGTH;i++){ips200.show_string(10,(i+1)*FONT_HEIGHT,Para4[i].label); ips200.show_int(120,(i+1)*FONT_HEIGHT,*Para4[i].value,4);}
    menu_draw_underline(menu_last_row*FONT_HEIGHT,RGB565_WHITE); menu_draw_underline(menu_temp_index*FONT_HEIGHT,RGB565_RED); menu_last_row=menu_temp_index;
}

void menu_draw_para5(void){
    ips200.clear(); ips200.show_string(0,0,"Para 5/5");
    for(int i=0;i<Para5_LENGTH;i++){ips200.show_string(10,(i+1)*FONT_HEIGHT,Para5[i].label); ips200.show_int(120,(i+1)*FONT_HEIGHT,*Para5[i].value,4);}
    menu_draw_underline(menu_last_row*FONT_HEIGHT,RGB565_WHITE); menu_draw_underline(menu_temp_index*FONT_HEIGHT,RGB565_RED); menu_last_row=menu_temp_index;
}

void menu_draw_camera(void){
    ips200.clear(); ips200.show_string(0,0,"Camera Para");
    for(int i=0;i<Cama1_LENGTH;i++){ips200.show_string(10,(i+1)*FONT_HEIGHT,Cama1[i].label); ips200.show_int(120,(i+1)*FONT_HEIGHT,*Cama1[i].value,4);}
    menu_draw_underline(menu_last_row*FONT_HEIGHT,RGB565_WHITE); menu_draw_underline(menu_temp_index*FONT_HEIGHT,RGB565_RED); menu_last_row=menu_temp_index;
}

//**************************************************************************
// 显示灰度图 + 标线
//**************************************************************************
void ShowGrayImgParams(const uint8_t *gray_img, uint8_t thresh)
{
    if(menu_page_num == PAGE_HOME)
    {
        ips200.show_gray_image(0,0,gray_img,IMAGE_WIDTH,IMAGE_HEIGHT,IMAGE_WIDTH,IMAGE_HEIGHT,thresh);

        for (uint8_t i = 0; i < real_time_int_LENGTH; i++)
        {
            ips200.show_string(10, FONT_HEIGHT*(i)+IMAGE_HEIGHT+20, real_time_int[i].label);
            ips200.show_int(70, FONT_HEIGHT*(i)+IMAGE_HEIGHT+20, *(real_time_int[i].value), 4);
        }
        for (uint8_t i = 0; i < real_time_float_LENGTH; i++)
        {
            ips200.show_string(110, FONT_HEIGHT*(i)+IMAGE_HEIGHT+20, real_time_float[i].label);
            ips200.show_float(170, FONT_HEIGHT*(i)+IMAGE_HEIGHT+20, *(real_time_float[i].value), 3,3);
        }

        int img_line = IMAGE_WIDTH / 10;
        int img_column = IMAGE_HEIGHT / 10;
        for(int i=0; i<=img_line; i++){
            if(i%2 ==0) ips200.draw_line(i*10,IMAGE_HEIGHT, i*10,IMAGE_HEIGHT+10, RGB565_PINK);
            else        ips200.draw_line(i*10,IMAGE_HEIGHT+5, i*10,IMAGE_HEIGHT+10, RGB565_PINK);
        }
        ips200.draw_line(0,IMAGE_HEIGHT+10,IMAGE_WIDTH+10,IMAGE_HEIGHT+10, RGB565_PINK);

        for(int i=0; i<=img_column; i++){
            if(i%2 ==0) ips200.draw_line(IMAGE_WIDTH, i*10, IMAGE_WIDTH+10, i*10, RGB565_PINK);
            else        ips200.draw_line(IMAGE_WIDTH+5, i*10, IMAGE_WIDTH+10, i*10, RGB565_PINK);
        }
        ips200.draw_line(IMAGE_WIDTH+10,0,IMAGE_WIDTH+10,IMAGE_HEIGHT+10, RGB565_PINK);
    }

    for(int i=0; i<IMAGE_HEIGHT; i++){
        ips200.draw_point(current_left_col[i], i, RGB565_PURPLE);
        ips200.draw_point(current_left_col[i]+1, i, RGB565_PURPLE);
        ips200.draw_point(current_right_col[i],i, RGB565_BLUE);
        ips200.draw_point(current_right_col[i]-1,i, RGB565_BLUE);
        ips200.draw_point(centralline[i],i, RGB565_GREEN);
        ips200.draw_point(centralline[i]-1,i, RGB565_GREEN);
    }
}

void ShowRGBImgParams(const uint16_t *rgb_img)
{
    if(menu_page_num == PAGE_HOME)
    {
        ips200.show_rgb565_image(0,0,rgb_img,IMAGE_WIDTH,IMAGE_HEIGHT,IMAGE_WIDTH,IMAGE_HEIGHT,0);
    }
}

// 启动小车
void start_standalone_task(void)
{
    // 菜单运行在2ms PIT线程中。这里只发请求，OpenCV图像和全部运行状态
    // 由主线程统一清理，避免中断线程与图像线程并发释放ROI。
    launch_request = 1;
}

void prepare_new_run(void)
{
    if(launch_request == 0) return;

    // PIT中断继续存在，但在清理窗口内不再读写控制状态。
    // 这样既能安全释放OpenCV ROI，也不会破坏PIT对象后续再次发车的能力。
    begin_runtime_reset_guard();
    car_running = 0;
    Motor_pwm_set(0, 0);
    PWM_FAN_SET(0);
    encoder_left.clear_count();
    encoder_right.clear_count();

    reset_image_runtime_state();
    reset_black_region_runtime_state();
    reset_ncnn_runtime_state();
    reset_control_runtime_state();
    reset_isr_runtime_state();

    menu_page_num = PAGE_HOME;
    menu_temp_index = 0;
    menu_last_row = 0;
    screen_clear_flag = 0;
    launch_request = 0;
    car_running = 1;
    end_runtime_reset_guard();

    printf("[START] 运行状态已清空，开始新一轮发车\n");
}

void sync_camera_exposure_config(void)
{
    camera_exposure_mode = (camera_exposure_mode == 0) ? 0 : 1;
    if (camera_manual_exposure <= 0) camera_manual_exposure = 140;

    UVC_AUTO_EXPOSURE_MODE = (camera_exposure_mode == 0) ?
                             UVC_AUTO_EXPOSURE_ENABLE :
                             UVC_AUTO_EXPOSURE_DISABLE;
    UVC_DEFAULT_EXPOSURE = camera_manual_exposure;
}

// 将菜单曝光配置实时下发给已打开的摄像头
void camera_reinit(void)
{
    sync_camera_exposure_config();

    if (uvc_dev.set_auto_exposure(UVC_AUTO_EXPOSURE_MODE) != 0)
    {
        std::cerr << "[UVC] 曝光模式切换失败: mode="
                  << camera_exposure_mode << std::endl;
        return;
    }

    if (camera_exposure_mode == 1)
    {
        if (uvc_dev.set_exposure_value(camera_manual_exposure) != 0)
        {
            std::cerr << "[UVC] 手动曝光值设置失败: "
                      << camera_manual_exposure << std::endl;
            return;
        }
        std::cout << "[UVC] 已切换手动曝光，曝光值="
                  << camera_manual_exposure << std::endl;
    }
    else
    {
        std::cout << "[UVC] 已切换自动曝光" << std::endl;
    }
}

//**************************************************************************
// 函数：menu_isr_handler
// 功能：完全按你的规则：
// 1. 选中行 = 下划线
// 2. 标题行 → 左右切页
// 3. 参数行 → 左右调参
// 4. 按键：松开触发
//**************************************************************************
// 全局标志：屏幕清空（只清一次）
extern uint8_t* gray_image;
// 封装函数：屏幕自动显示 / 黑屏管理（无输入参数）
void screen_auto_display(void)
{
    // 停车 → 显示图像 + UI
    if(car_running == 0)
    {
        if(menu_page_num == PAGE_HOME)
        {
            if(im_dis_type==0)ShowGrayImgParams(gray_image, 0);
            if(im_dis_type==1)ShowGrayImgParams(gray_image, image_threshold);
            if(Target_flag==1)draw_rect_box(target_x1, target_y1, target_x2, target_y2, RGB565_RED);
            // if(Brick_flag==1)draw_rect_box(target_x1, target_y1, target_x2, target_y2, RGB565_GREEN);
            // if(br_found==1)draw_rect_box(target_x1, target_y1, target_x2, target_y2, RGB565_RED);
        }
        screen_clear_flag = 0; // 允许下次启动时清屏
    }
    // 运行 → 只黑屏一次(不管在哪个页面)
    else
    {
        if(screen_clear_flag == 0)
        {
            ips200.clear();
            screen_clear_flag = 1;
        }
    }
}
void menu_isr_handler(void)
{
    uint8_t k1 = key1_short_press_detect();   // 上
    uint8_t k2 = key2_short_press_detect();   // 下
    uint8_t k3_event = key3_press_detect();   // 短按=左，长按=确认/返回
    uint8_t k4 = key4_short_press_detect();   // 右

    // ======================= 按键电平反转 =======================
    k1 = !k1;
    k2 = !k2;
    // k3_event: 0=无事件, 1=短按, 2=长按（已是正逻辑，无需反转）
    uint8_t k3 = (k3_event == 1) ? 1 : 0;    // 短按 → 原K3（左/减）
    uint8_t k5 = (k3_event == 2) ? 1 : 0;    // 长按 → 原K5（确认/返回）
    k4 = !k4;

    // ======================= 【问题1】这里会导致切页后瞬间黑屏不刷新 =======================
    // 现象：屏幕不高刷、文字消失
    // 修复：不能直接return，否则刚切页没按键就不画菜单
    // if(k1 == 0 && k2 == 0 && k3 == 0 && k4 == 0 && k5 == 0)
    // {
    //     return;  
    // }

    // 最大可选中行
    int max_row = 0;
    if(menu_page_num == PAGE_HOME)        max_row = 2;
    else if(menu_page_num == PAGE_PARA1)  max_row = Para1_LENGTH;
    else if(menu_page_num == PAGE_PARA2)  max_row = Para2_LENGTH;
    else if(menu_page_num == PAGE_PARA3)  max_row = Para3_LENGTH;
    else if(menu_page_num == PAGE_PARA4)  max_row = Para4_LENGTH;
    else if(menu_page_num == PAGE_PARA5)  max_row = Para5_LENGTH;
    else if(menu_page_num == PAGE_CAMA1)  max_row = Cama1_LENGTH;

    // ==================== 上下选择（全页面通用） ====================
    if(k1)
    {
        menu_temp_index--;
        if(menu_temp_index < 0) menu_temp_index = 0;
    }
    if(k2)
    {
        menu_temp_index++;
        if(menu_temp_index > max_row) menu_temp_index = max_row;
    }

    // ==================== 首页逻辑 ====================
    if(menu_page_num == PAGE_HOME)
    {
        if(k4)
        {
            // ======================= 【问题2】先改页+重置index，再clear =======================
            // 原顺序错误：先clear → 画面闪、文字丢失
            // 修复：先修改变量 → 再清屏
            if(menu_temp_index == 1)
            {
                menu_page_num = PAGE_PARA1;
                menu_temp_index = 0;  // 先复位
                ips200.clear();       // 后清屏
            }
            if(menu_temp_index == 2)
            {
                menu_page_num = PAGE_CAMA1;
                menu_temp_index = 0;  // 先复位
                ips200.clear();       // 后清屏
            }
        }
    }
    // ==================== 参数页 / 相机页逻辑 ====================
    else
    {
        // ==================== 选中【标题行】：左右 = 切页 ====================
        if(menu_temp_index == 0)
        {
            if(k4)  // 右键 → 下一页
            {
                if(menu_page_num < PAGE_PARA5)
                {
                    menu_page_num++;
                    menu_temp_index = 0;
                    ips200.clear();
                }
            }

            if(k3)  // 左键 → 上一页
            {
                if(menu_page_num == PAGE_CAMA1)
                {
                    menu_page_num = PAGE_HOME;
                    Parameter_Write();
                    camera_reinit();
                }
                else
                {
                    menu_page_num--;
                    if(menu_page_num < PAGE_PARA1)
                    {
                        menu_page_num = PAGE_HOME;
                        Parameter_Write();
                    }
                }
                menu_temp_index = 0;
                ips200.clear();
            }
        }
        // ==================== 选中【参数行】：左右 = 调参 ====================
        else
        {
            int idx = menu_temp_index - 1;
            if(k4)  // 右键 +
            {
                if(menu_page_num == PAGE_PARA1 && idx < Para1_LENGTH) (*Para1[idx].value)++;
                if(menu_page_num == PAGE_PARA2 && idx < Para2_LENGTH) (*Para2[idx].value)++;
                if(menu_page_num == PAGE_PARA3 && idx < Para3_LENGTH) (*Para3[idx].value)++;
                if(menu_page_num == PAGE_PARA4 && idx < Para4_LENGTH) (*Para4[idx].value)++;
                if(menu_page_num == PAGE_PARA5 && idx < Para5_LENGTH) (*Para5[idx].value)++;
                if(menu_page_num == PAGE_CAMA1 && idx < Cama1_LENGTH)
                {
                    if(idx == 0) camera_exposure_mode = 1;
                    else if(idx == 1) camera_manual_exposure += 5;
                    else (*Cama1[idx].value)++;
                }
            }
            if(k3)  // 左键 -
            {
                if(menu_page_num == PAGE_PARA1 && idx < Para1_LENGTH) (*Para1[idx].value)--;
                if(menu_page_num == PAGE_PARA2 && idx < Para2_LENGTH) (*Para2[idx].value)--;
                if(menu_page_num == PAGE_PARA3 && idx < Para3_LENGTH) (*Para3[idx].value)--;
                if(menu_page_num == PAGE_PARA4 && idx < Para4_LENGTH) (*Para4[idx].value)--;
                if(menu_page_num == PAGE_PARA5 && idx < Para5_LENGTH) (*Para5[idx].value)--;
                if(menu_page_num == PAGE_CAMA1 && idx < Cama1_LENGTH)
                {
                    if(idx == 0) camera_exposure_mode = 0;
                    else if(idx == 1)
                    {
                        camera_manual_exposure -= 5;
                        if(camera_manual_exposure < 5) camera_manual_exposure = 5;
                    }
                    else (*Cama1[idx].value)--;
                }
            }
        }
    }

    // CdOff 是二值开关，防止通用“加/减1”菜单把它调到有效范围之外。
    if (card_recognition_disable < 0) card_recognition_disable = 0;
    if (card_recognition_disable > 1) card_recognition_disable = 1;
    camera_exposure_mode = (camera_exposure_mode == 0) ? 0 : 1;
    if (camera_manual_exposure < 5) camera_manual_exposure = 5;
    // Card1~Card5 同样只允许0/1。
    for (int i = 0; i < CARD_SEQUENCE_COUNT; ++i)
    {
        if (card_sequence_enable[i] < 0) card_sequence_enable[i] = 0;
        if (card_sequence_enable[i] > 1) card_sequence_enable[i] = 1;
    }

    // ==================== K5：启动 / 保存返回 ====================
    if(k5)
    {
        if(menu_page_num == PAGE_HOME)
        {
            start_standalone_task();
        }
        else
        {
            // <====== 新增：判断如果当前是相机页，则在退出前执行初始化
            if(menu_page_num == PAGE_CAMA1) 
            {
                camera_reinit(); 
            }
            Parameter_Write();
            menu_page_num = PAGE_HOME;
            ips200.clear();
            menu_temp_index = 0;
        }
    }

    // ==================== 刷新菜单（只在静止时刷新） ====================
    // 修复：无论有没有按键，都执行刷新 → 保证屏幕一直高刷
    if(car_running == 0)
    {
        switch(menu_page_num)
        {
            case PAGE_HOME:    menu_draw_home();     break;
            case PAGE_PARA1:   menu_draw_para1();    break;
            case PAGE_PARA2:   menu_draw_para2();    break;
            case PAGE_PARA3:   menu_draw_para3();    break;
            case PAGE_PARA4:   menu_draw_para4();    break;
            case PAGE_PARA5:   menu_draw_para5();    break;
            case PAGE_CAMA1:   menu_draw_camera();   break;
        }
    }
}
