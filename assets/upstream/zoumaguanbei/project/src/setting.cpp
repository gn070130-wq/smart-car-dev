#include "zf_common_headfile.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include "pwm.h"

using json = nlohmann::json;
using namespace std;

// 外部变量声明保持不变
extern int key_scan_gpio;
extern int zebra_cold_time;
extern int fast_speed, mid_speed, barrier_speed, slow_speed;
extern float kp, kd;
extern int curve_far_num, far_num, close_num;
extern float dif_speed;
extern int pwm_servo, round_num, barrier_start_num, barrier_end_num, barrier_hold, zebra_hold;
extern float kp_dif, kd_dif, wp, wi, wd, gp;
extern int exposure, auto_exposure;
extern float image_scale;
extern double camera_ipm_pitch;
extern double camera_ipm_mat[3][3];
extern void camera_exposure_setting();
extern void camera_ipm_set_pitch(double pitch);

int setting_thing[20] = {0};
int add_sum = 1;
int back_flag = 0;
int pwm_fan = 0;
int size_frame = 0;

// 获取文件路径辅助函数
string get_config_path(int file_sum) {
    return "/home/root/configs/config" + to_string(file_sum) + ".json";
}

// 保存配置到 JSON
void start_save(int file_sum) {
    // 检查并创建目录
    mkdir("/home/root/configs", 0777);

    json j;
    j["slow_speed"] = slow_speed;
    j["barrier_speed"] = barrier_speed;
    j["mid_speed"] = mid_speed;
    j["fast_speed"] = fast_speed;
    j["kp"] = kp;
    j["kd"] = kd;
    j["barrier_hold"] = barrier_hold;
    j["curve_far_num"] = curve_far_num;
    j["far_num"] = far_num;
    j["zebra_hold"] = zebra_hold;
    j["pwm_fan"] = pwm_fan;
    j["round_num"] = round_num;
    j["barrier_end_num"] = barrier_end_num;
    j["barrier_start_num"] = barrier_start_num;
    j["kp_dif"] = kp_dif;
    j["kd_dif"] = kd_dif;
    j["gp"] = gp;
    //j["wi"] = wi;
    //j["wd"] = wd;
    j["exposure"] = exposure;
    j["auto_exposure"] = auto_exposure;
    j["image_scale"] = image_scale;

    ofstream outfile(get_config_path(file_sum));
    if (outfile.is_open()) {
        outfile << j.dump(4); // 缩进4格，方便人类阅读
        outfile.close();
    }
}

// 从 JSON 读取配置
void start_load(int file_sum) {
    string path = get_config_path(file_sum);
    ifstream infile(path);

    if (!infile.is_open()) {
        cout << "Config not found, creating default: " << path << endl;
        start_save(file_sum); // 不存在则保存当前值作为默认
        return;
    }

    try {
        json j;
        infile >> j;
        
        // 使用 value 带有默认值的获取方式，防止 JSON 缺少某个键导致崩溃
        slow_speed = j.value("slow_speed", slow_speed);
        barrier_speed = j.value("barrier_speed", barrier_speed);
        mid_speed = j.value("mid_speed", mid_speed);
        fast_speed = j.value("fast_speed", fast_speed);
        kp = j.value("kp", kp);
        kd = j.value("kd", kd);
        barrier_hold = j.value("barrier_hold", barrier_hold);
        curve_far_num = j.value("curve_far_num", curve_far_num);
        far_num = j.value("far_num", far_num);
        zebra_hold = j.value("zebra_hold", zebra_hold);
        pwm_fan = j.value("pwm_fan", pwm_fan);
        round_num = j.value("round_num", round_num);
        barrier_end_num = j.value("barrier_end_num", barrier_end_num);
        barrier_start_num = j.value("barrier_start_num", barrier_start_num);
        kp_dif = j.value("kp_dif", kp_dif);
        kd_dif = j.value("kd_dif", kd_dif);
        gp = j.value("gp", gp);
        //wi = j.value("wi", wi);
        //wd = j.value("wd", wd);
        exposure = j.value("exposure", exposure);
        auto_exposure = j.value("auto_exposure", auto_exposure);
        image_scale = j.value("image_scale", image_scale);

    } catch (const exception& e) {
        cerr << "JSON Parse Error: " << e.what() << endl;
    }
    infile.close();
}

// 修改后的 setting1 (新增 file_sum 参数)
void setting1(int list_num, int file_sum) {
    // 启动前先加载
    start_load(file_sum);

    setting_thing[1] = slow_speed;
    setting_thing[2] = mid_speed;
    setting_thing[3] = fast_speed;
    //setting_thing[4] = (int)(wi * 1000);
    setting_thing[4] = barrier_speed;
    setting_thing[5] = (int)(wd * 1000);
    setting_thing[6] = barrier_start_num;
    setting_thing[7] = barrier_end_num;
    back_flag = 0;
    setting_display(1, list_num);

    while(1) {
        key_scan_gpio = keyboard_scan_cst816();
        if(key_scan_gpio == -1) continue;
        
        if(key_scan_gpio == DOWN)  setting_thing[list_num] -= add_sum;
        else if(key_scan_gpio == UP)    setting_thing[list_num] += add_sum;
        else if(key_scan_gpio == RIGHT) { if(add_sum < 10000) add_sum *= 10; }
        else if(key_scan_gpio == LEFT)  { if(add_sum > 1) add_sum /= 10; }
        else if(key_scan_gpio == ESC)   back_flag = 1;

        slow_speed = setting_thing[1];
        mid_speed = setting_thing[2];
        fast_speed = setting_thing[3];
        barrier_speed = setting_thing[4];
        wd = (float)setting_thing[5] / 1000.0f;
        barrier_start_num = setting_thing[6];
        barrier_end_num = setting_thing[7];
        setting_display(1, list_num);
        usleep(200000);

        if(back_flag == 1) {
            start_save(file_sum); // 退出时保存
            key_scan_gpio = 0;
            break;
        }
    }
}

// 修改后的 setting2
void setting2(int list_num, int file_sum) {
    start_load(file_sum);

    setting_thing[1] = barrier_hold;
    setting_thing[2] = curve_far_num;
    setting_thing[3] = far_num;
    setting_thing[4] = zebra_hold;
    setting_thing[5] = pwm_fan;
    setting_thing[6] = round_num;
    setting_thing[7] = (int)(image_scale * 1000);
    
    back_flag = 0;
    setting_display(2, list_num);

    while(1) {
        key_scan_gpio = keyboard_scan_cst816();
        if(key_scan_gpio == -1) continue;

        switch (key_scan_gpio) {
            case DOWN:  setting_thing[list_num] -= add_sum; break;
            case UP:    setting_thing[list_num] += add_sum; break;
            case RIGHT: if(add_sum < 10000) add_sum *= 10; break;
            case LEFT:  if(add_sum > 1) add_sum /= 10; break;
            case ESC:   back_flag = 1; break;
        }

        barrier_hold = setting_thing[1];
        curve_far_num = setting_thing[2];
        far_num = setting_thing[3];
        zebra_hold = setting_thing[4];
        pwm_fan = setting_thing[5];
        round_num = setting_thing[6];
        image_scale = (float)setting_thing[7] / 1000.0f;

        pwm_set_duty(SERVO, pwm_servo);
        setting_display(2, list_num);
        usleep(200000);

        if(back_flag == 1) {
            start_save(file_sum);
            key_scan_gpio = 0;
            break;
        }
    }
}

// 修改后的 setting3
void setting3(int list_num, int file_sum) {
    start_load(file_sum);

    setting_thing[1] = (int)(kp_dif * 1000);
    setting_thing[2] = (int)(kd_dif * 1000);
    setting_thing[3] = (int)(gp * 1000);
    setting_thing[4] = (int)(kp * 1000);
    setting_thing[5] = (int)(kd * 1000);
    setting_thing[6] = exposure;
    setting_thing[7] = auto_exposure;

    back_flag = 0;
    setting_display(3, list_num);

    while (1)
    {
        key_scan_gpio = keyboard_scan_cst816();
        if(key_scan_gpio == -1) continue;

        switch (key_scan_gpio) {
            case DOWN:  setting_thing[list_num] -= add_sum; break;
            case UP:    setting_thing[list_num] += add_sum; break;
            case RIGHT: if(add_sum < 10000) add_sum *= 10; break;
            case LEFT:  if(add_sum > 1) add_sum /= 10; break;
            case ESC:   back_flag = 1; break;
        }

        kp_dif = (float)setting_thing[1] / 1000.0f;
        kd_dif = (float)setting_thing[2] / 1000.0f;
        gp = (float)setting_thing[3] / 1000.0f;
        kp = (float)setting_thing[4] / 1000.0f;
        kd = (float)setting_thing[5] / 1000.0f;
        exposure = setting_thing[6];
        auto_exposure = setting_thing[7];
        
        setting_display(3, list_num);
        //camera_exposure_setting();
        usleep(200000);

        if(back_flag == 1) {
            start_save(file_sum);
            key_scan_gpio = 0;
            break;
        }
    }
}

// 逆透视微调：默认模式调透视和整体缩放，ENTER 切换到整体平移模式。
// 同步补偿 M[0][1]/M[1][1] 锚定底部中心，避免横向和纵向整体漂移。
void camera_setting()
{
    camera_cfg_load();
    bool translate_mode = false;
    cout << "IPM adjust: UP/DOWN pitch, LEFT/RIGHT scale, ENTER translate mode, CST816 ESC exit" << endl;

    while (1)
    {
        get_mmap_image(1);
        print_image_toushi(0);
        // 调试辅助线：保留现有显示，不参与逆透视参数计算。
        for(int i = 0; i < 240; i++) {
            print_pixel(i, 109, "blue");
            print_pixel(i, 209, "red");
        }

        int touch_key = keyboard_scan_cst816();
        if (touch_key == ESC)
        {
            key_scan_gpio = 0;
            break;
        }

        int gpio_key = keyboard_scan_gpio();
        bool changed = false;

        if (gpio_key == ENTER)
        {
            translate_mode = !translate_mode;
            cout << (translate_mode
                ? "IPM mode: translate (arrows move image)"
                : "IPM mode: pitch/scale (UP/DOWN pitch, LEFT/RIGHT scale)") << endl;
            usleep(200000);
            continue;
        }

        if (translate_mode)
        {
            const int move_step = 1;
            if (gpio_key == LEFT)       { camera_ipm_offset_x -= move_step; changed = true; }
            else if (gpio_key == RIGHT) { camera_ipm_offset_x += move_step; changed = true; }
            else if (gpio_key == UP)    { camera_ipm_offset_y -= move_step; changed = true; }
            else if (gpio_key == DOWN)  { camera_ipm_offset_y += move_step; changed = true; }
        }
        else
        {
            if (gpio_key == UP || gpio_key == DOWN)
            {
                const double pitch_step = 0.000010;
                if (gpio_key == UP) camera_ipm_pitch += pitch_step;
                else camera_ipm_pitch -= pitch_step;
                camera_ipm_set_pitch(camera_ipm_pitch);
                changed = true;
            }
            else if (gpio_key == LEFT || gpio_key == RIGHT)
            {
                const double scale_step = 0.005;
                if (gpio_key == RIGHT) camera_ipm_scale += scale_step;
                else camera_ipm_scale -= scale_step;
                if (camera_ipm_scale < 0.5) camera_ipm_scale = 0.5;
                if (camera_ipm_scale > 2.0) camera_ipm_scale = 2.0;
                changed = true;
            }
        }

        if (changed)
        {
            camera_cfg_save();
            ImagePerspective2_Init();
            ImagePerspective2_Color_Init();
            ImagePerspective3_Color_Init();

            cout << fixed << setprecision(6)
                 << "pitch=" << camera_ipm_pitch
                 << " scale=" << camera_ipm_scale
                 << " offset=(" << camera_ipm_offset_x << "," << camera_ipm_offset_y << ")"
                 << endl;
            usleep(200000);
        }
    }
}
