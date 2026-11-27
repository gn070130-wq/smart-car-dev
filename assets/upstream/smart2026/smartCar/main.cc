#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <csignal>
#include <opencv2/opencv.hpp>

#include "headfile.h"
#include "param/param.hpp"
#include "yolov5_detect.h"

//全局

static CarConfig g_cfg;
static const char* CONFIG_PATH = "./param/config.json";
static std::atomic<bool> g_running{true};
static std::atomic<bool> g_started{false};  

static void signal_handler(int) {
    std::cout << "\n[INFO] 收到退出信号\n";
    g_running.store(false);
}

//帧共享 

static std::mutex  g_frame_mutex;
static cv::Mat     g_latest_frame;
static bool        g_frame_ready = false;
static std::atomic<int> g_seed_hint_x{-1};

//传感器共享

struct SensorData {
    std::mutex mtx;
    float    accel_x = 0.0f, accel_y = 0.0f, accel_z = 1.0f;
    float    gyro_x = 0.0f,  gyro_y = 0.0f,  gyro_z = 0.0f;
    uint16_t tof_mm  = 8192;
    int32_t  enc1 = 0, enc2 = 0;
};
static SensorData g_sensor;

//硬件对象

static ICM42688     g_imu;
static VL53L0X      g_tof;
static Buzzer       g_buzzer;
static YoloDetector g_yolo;

//采集线程 

struct CaptureContext { cv::VideoCapture* cap; };

static void capture_callback(void* arg)
{
    auto* ctx = static_cast<CaptureContext*>(arg);
    cv::Mat img;
    if (!ctx->cap->read(img) || img.empty()) return;
    std::lock_guard<std::mutex> lk(g_frame_mutex);
    g_latest_frame = img;
    g_frame_ready  = true;
}

//传感器线程 

struct SensorContext { bool imu_ok, tof_ok, enc_ok; };

static void sensor_callback(void* arg)
{
    auto* ctx = static_cast<SensorContext*>(arg);

    if (ctx->imu_ok) {
        g_imu.upDataAcc();
        g_imu.upDataGyro();
        g_imu.thread_syn();
    }
    if (ctx->tof_ok) {
        g_tof.upData();
        g_tof.thread_syn();
    }
    if (ctx->enc_ok) {
        car_update_encoders();
    }

    std::lock_guard<std::mutex> lk(g_sensor.mtx);
    if (ctx->imu_ok) {
        g_sensor.accel_x = g_imu.syn_accel_x;
        g_sensor.accel_y = g_imu.syn_accel_y;
        g_sensor.accel_z = g_imu.syn_accel_z;
        g_sensor.gyro_x  = g_imu.syn_gyro_x;
        g_sensor.gyro_y  = g_imu.syn_gyro_y;
        g_sensor.gyro_z  = g_imu.syn_gyro_z;
    }
    if (ctx->tof_ok) {
        g_sensor.tof_mm = g_tof.syn_vl53l0x_distance_mm;
    }
    if (ctx->enc_ok) {
        g_sensor.enc1 = car_get_encoder1();
        g_sensor.enc2 = car_get_encoder2();
    }
}

//控制线程 

struct ControlContext {
    IPMConfig*                ipm_cfg;
    cv::Mat*                  M;
    PIDController*            steer_pid;
    SceneState*               scene_state;
    TransmissionStreamServer* server;

    int     lost_frame_cnt   = 0;
    int     steer_out_last   = 0;
    int     yolo_skip_cnt    = 0;
    int     yolo_skip_interval = 5;
    int64_t t_last_tick      = 0;
    int     fps_cnt          = 0;
};

static void control_callback(void* arg)
{
    auto* ctx = static_cast<ControlContext*>(arg);

    //未发车时只做图传预览，不输出电机
    if (!g_started.load()) {
        cv::Mat img;
        {
            std::lock_guard<std::mutex> lk(g_frame_mutex);
            if (!g_frame_ready || g_latest_frame.empty()) return;
            img = g_latest_frame.clone();
        }
        if (g_cfg.debug.debug_stream && ctx->server->is_running()) {
            ctx->server->update_frame_mat(img);
        }
        return;
    }

    //取帧
    cv::Mat img;
    {
        std::lock_guard<std::mutex> lk(g_frame_mutex);
        if (!g_frame_ready || g_latest_frame.empty()) return;
        img = g_latest_frame.clone();
    }

    //传感器
    float ax, ay, az;
    uint16_t tof;
    {
        std::lock_guard<std::mutex> lk(g_sensor.mtx);
        ax = g_sensor.accel_x; ay = g_sensor.accel_y; az = g_sensor.accel_z;
        tof = g_sensor.tof_mm;
    }

    //预处理 + IPM 
    cv::Mat img_bin, img_ipm;
    preprocess(img, img_bin, g_seed_hint_x.load());
    ipm_warp(img_bin, img_ipm, *(ctx->M));

    //巡线
    LineInfo line_info;
    line_info_reset(line_info);
    maze_detect(img_ipm, line_info, g_cfg.line);
    detect_straight(line_info, g_cfg.line);

    // YOLO（仅在红色矩形出现且未识别时触发）
    if (g_yolo.is_loaded()
        && ctx->scene_state->target_red_rect_seen
        && ctx->scene_state->target_category == TARGET_NONE)
    {
        ctx->yolo_skip_cnt++;
        if (ctx->yolo_skip_cnt >= ctx->yolo_skip_interval) {
            ctx->yolo_skip_cnt = 0;
            std::vector<DetectObject> objects;
            if (g_yolo.detect(img, objects,
                              g_cfg.yolo.prob_thresh,
                              g_cfg.yolo.nms_thresh) == 0) {
                TargetCategory cat = g_yolo.classify_target(objects);
                if (cat != TARGET_NONE) {
                    ctx->scene_state->target_category = cat;
                }
            }
        }
    }

    //场景检测 
    SceneType scene = detect_scene(img_ipm, img, line_info,
                                   *(ctx->scene_state), g_cfg.scene,
                                   g_cfg.line.row_scan_start,
                                   ax, ay, az, tof);

    //PID 参数切换（直道/弯道）
    if (line_info.is_straight) {
        pid_tune(ctx->steer_pid,
                 g_cfg.pid_straight.kp, g_cfg.pid_straight.ki, g_cfg.pid_straight.kd);
    } else {
        pid_tune(ctx->steer_pid,
                 g_cfg.pid_curve.kp, g_cfg.pid_curve.ki, g_cfg.pid_curve.kd);
    }


    int  drive_speed = g_cfg.speed.base_speed;
    int  steer_out   = 0;
    bool do_drive    = true;

    if (line_info.found) {
        ctx->lost_frame_cnt = 0;

        //更新 Floodfill 种子
        int bot_r = g_cfg.line.row_scan_start;
        if (bot_r < IMG_H && line_info.road_width[bot_r] > 0) {
            g_seed_hint_x.store(line_info.center_x[bot_r]);
        }

        //统一 PID 计算：error + bypass_offset */
        float adjusted_error = line_info.error
                             + ctx->scene_state->bypass_error_offset;

        switch (scene) {

        case SCENE_INTERSECTION:
            /* 十字路口：清积分强制直行 */
            pid_reset(ctx->steer_pid);
            steer_out   = 0;
            drive_speed = g_cfg.speed.speed_inter;
            break;

        case SCENE_ROUNDABOUT:
        case SCENE_ROUNDABOUT_IN:
            /* 环岛：PID + offset（offset在状态机中设置） */
            steer_out   = (int)pid_compute(ctx->steer_pid, adjusted_error);
            drive_speed = g_cfg.speed.speed_curve;
            break;

        case SCENE_OBSTACLE:
            /* 路障：PID + offset（不再杀PID，弯道也能绕） */
            steer_out   = (int)pid_compute(ctx->steer_pid, adjusted_error);
            drive_speed = g_cfg.speed.speed_obstacle;
            break;

        case SCENE_SLOPE:
            /* 坡道：正常PID，减速 */
            steer_out   = (int)pid_compute(ctx->steer_pid, adjusted_error);
            drive_speed = g_cfg.scene.speed_slope;
            break;

        case SCENE_TARGET_BYPASS:
            /* 目标板绕行：PID + offset */
            steer_out   = (int)pid_compute(ctx->steer_pid, adjusted_error);
            drive_speed = g_cfg.scene.speed_target;
            break;

        case SCENE_ZEBRA:
            /* 斑马线：正常巡线 */
            steer_out   = (int)pid_compute(ctx->steer_pid, adjusted_error);
            drive_speed = g_cfg.speed.base_speed;

            /* 第2次经过 = 终点 */
            if (ctx->scene_state->zebra_pass_count >= 2) {
                car_stop();
                do_drive = false;
                g_running.store(false);
                g_buzzer.set_duty_freq(50, 1000);
                std::cout << "[INFO] 终点停车! 经过斑马线 "
                          << ctx->scene_state->zebra_pass_count << " 次\n";
            }
            break;

        default: /* SCENE_NORMAL */
            steer_out = (int)pid_compute(ctx->steer_pid, adjusted_error);

            /* 动态调速 */
            if (line_info.is_straight) {
                drive_speed = g_cfg.speed.base_speed;
            } else {
                float abs_err = std::abs(line_info.error);
                if      (abs_err > 40.0f) drive_speed = g_cfg.speed.speed_curve;
                else if (abs_err > 20.0f) drive_speed = (g_cfg.speed.base_speed +
                                                          g_cfg.speed.speed_curve) / 2;
                else                      drive_speed = g_cfg.speed.base_speed;
            }
            break;
        }

        ctx->steer_out_last = steer_out;

    } else {
        /* ── 丢线处理 ── */
        ctx->lost_frame_cnt++;
        pid_reset(ctx->steer_pid);
        g_seed_hint_x.store(-1);

        if (ctx->lost_frame_cnt <= 10) {
            steer_out   = ctx->steer_out_last;
            drive_speed = g_cfg.speed.speed_lost;
        } else if (ctx->lost_frame_cnt <= 30) {
            steer_out   = 0;
            drive_speed = g_cfg.speed.speed_lost / 2;
        } else {
            car_stop();
            do_drive = false;
        }
    }

    if (do_drive) {
        car_set_speed_diff(drive_speed + steer_out,
                           drive_speed - steer_out);
    }

    /* ── 图传 ── */
    if (g_cfg.debug.debug_stream && ctx->server->is_running()) {
        cv::Mat dbg;
        cv::cvtColor(img_ipm, dbg, cv::COLOR_GRAY2BGR);
        draw_line_result(dbg, line_info, g_cfg.line);

        const char* sn[] = {"NORM","INTER","ROUND","RDIN",
                            "OBS","SLOPE","ZEBRA","TGT","BYPASS"};
        char buf[128];
        snprintf(buf, sizeof(buf), "%s spd:%d st:%d ofs:%.0f %s z:%d",
                 sn[(int)scene], drive_speed, steer_out,
                 ctx->scene_state->bypass_error_offset,
                 line_info.is_straight ? "ST" : "CV",
                 ctx->scene_state->zebra_pass_count);
        cv::putText(dbg, buf, cv::Point(2, dbg.rows - 4),
                    cv::FONT_HERSHEY_PLAIN, 0.6, cv::Scalar(255, 200, 0), 1);
        ctx->server->update_frame_mat(dbg);
    }

    /* ── FPS ── */
    if (g_cfg.debug.debug_print_fps) {
        ctx->fps_cnt++;
        int64_t t = cv::getTickCount();
        if (ctx->t_last_tick > 0) {
            double dt = (double)(t - ctx->t_last_tick) / cv::getTickFrequency();
            if (dt >= 1.0) {
                std::cout << "[FPS] " << ctx->fps_cnt << "\n";
                ctx->fps_cnt = 0;
                ctx->t_last_tick = t;
            }
        } else {
            ctx->t_last_tick = t;
        }
    }
}

int main()
{
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    /* 1. 配置 */
    load_config(g_cfg, CONFIG_PATH);
    int img_w = g_cfg.camera.img_width;
    int img_h = g_cfg.camera.img_height;

    /* 2. 图传 */
    TransmissionStreamServer server;
    server.start_server(g_cfg.debug.transmission_port);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    /* 3. 摄像头 */
    cv::VideoCapture cap(0);
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  img_w);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, img_h);
    cap.set(cv::CAP_PROP_FPS, g_cfg.camera.fps);
    if (!cap.isOpened()) {
        std::cerr << "[ERROR] 摄像头打开失败\n";
        return -1;
    }

    /* 4. 电机 */
    bool motor_ok = (car_init() >= 0);

    /* 5. IMU */
    bool imu_ok = (g_imu.icm42688_init() == 0);

    /* 6. TOF */
    bool tof_ok = (g_tof.tof_init() == 0);

    /* 7. 蜂鸣器 */
    bool buzzer_ok = (g_buzzer.buzzer_init() == 0);

    /* 8. YOLO */
    if (g_cfg.yolo.enable) {
        g_yolo.load_model(g_cfg.yolo.param_path, g_cfg.yolo.model_path);
    }

    /* 9. IPM */
    IPMConfig ipm_cfg;
    ipm_init(ipm_cfg, g_cfg);
    cv::Mat M = get_ipm_matrix(ipm_cfg);

    /* 10. PID */
    PIDController steer_pid;
    pid_init(&steer_pid,
             g_cfg.pid_straight.kp, g_cfg.pid_straight.ki, g_cfg.pid_straight.kd,
             g_cfg.pid_straight.i_limit, g_cfg.pid_straight.out_limit, false);

    /* 11. 场景 */
    SceneState scene_state;
    scene_state_init(scene_state);

    /* 12. 线程 */
    CaptureContext cap_ctx{&cap};
    SensorContext  sns_ctx{imu_ok, tof_ok, motor_ok};

    ControlContext ctl_ctx;
    ctl_ctx.ipm_cfg     = &ipm_cfg;
    ctl_ctx.M           = &M;
    ctl_ctx.steer_pid   = &steer_pid;
    ctl_ctx.scene_state = &scene_state;
    ctl_ctx.server      = &server;

    TimerThread cap_thread(capture_callback, &cap_ctx,  10);
    TimerThread sns_thread(sensor_callback,  &sns_ctx,   5);
    TimerThread ctl_thread(control_callback, &ctl_ctx,  20);

    cap_thread.start();
    sns_thread.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ctl_thread.start();

    std::cout << "\n[INFO] 系统就绪 | IMU:" << (imu_ok?"OK":"X")
              << " TOF:" << (tof_ok?"OK":"X")
              << " YOLO:" << (g_yolo.is_loaded()?"OK":"X") << "\n";
    std::cout << "[INFO] 图传: http://<IP>:" << g_cfg.debug.transmission_port << "\n";


    if (buzzer_ok) {
        // 两短一长 = 就绪
        g_buzzer.set_duty_freq(50, 800);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        g_buzzer.set_duty(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        g_buzzer.set_duty_freq(50, 800);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        g_buzzer.set_duty(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        g_buzzer.set_duty_freq(50, 1200);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        g_buzzer.set_duty(0);
    }
    
    Key key;
    key.key_init();
    std::cout << "[INFO] 按下按键发车...\n";
    key.key_listeners();  // 阻塞等待
    g_started.store(true);

    /* 13. 等待完赛或 Ctrl+C */
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /* 清理 */
    car_stop();
    if (buzzer_ok) g_buzzer.set_duty(0);
    cap.release();
    server.stop_server();
    std::cout << "[INFO] 程序退出\n";
    return 0;
}
