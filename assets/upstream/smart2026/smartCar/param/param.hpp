#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "../include/json.hpp"

using json = nlohmann::json;

/* ─────────────────────── 子结构体 ─────────────────────── */

struct PIDParam {
    float kp = 0.9f, ki = 0.005f, kd = 0.6f;
    float i_limit = 40.0f, out_limit = 100.0f;
};

struct IPMParam {
    float src[4][2] = {
        {15.0f,115.0f},{145.0f,115.0f},{105.0f,66.0f},{55.0f,66.0f}
    };
    float dst[4][2] = {
        {20.0f,119.0f},{140.0f,119.0f},{140.0f,0.0f},{20.0f,0.0f}
    };
};

struct LineParam {
    int   row_scan_start    = 110;
    int   row_scan_end      =  20;
    int   road_width_min    =  15;
    int   search_radius     =  40;
    float straight_var_thresh = 5.0f;
};

struct SceneParam {
    /* 十字路口 */
    int   inter_width_thresh   = 100;
    int   inter_confirm_frames =   3;
    int   inter_pass_frames    =  25;

    /* 红色障碍物 */
    float obstacle_red_ratio   = 0.05f;
    int   obstacle_pass_frames =  30;
    int   obstacle_steer       =  35;     

    /* 斑马线 */
    int   zebra_stripe_min     =   4;
    int   zebra_confirm_frames =   3;     
    int   zebra_cooldown_frames =  40;   

    /* 环岛 */
    int   roundabout_confirm_frames = 3;
    int   roundabout_entry_frames   = 15;
    int   roundabout_pass_frames    = 80;
    int   roundabout_steer          = 30;

    /* 坡道 */
    int   slope_pass_frames = 60;
    int   speed_slope       = 30;

    /* 目标板 */
    int   target_bypass_frames = 40;
    int   target_steer         = 30;
    int   speed_target         = 25;
};

struct SpeedParam {
    int base_speed     = 50;
    int speed_curve    = 38;
    int speed_inter    = 35;
    int speed_obstacle = 25;
    int speed_lost     = 20;
};

struct CameraParam {
    int img_width = 160, img_height = 120, fps = 60;
};

struct DebugParam {
    bool debug_stream    = true;
    bool debug_print_fps = false;
    int  transmission_port = 8080;
};

struct SpeedPIDParam {
    bool  enable = false;
    float kp = 0.5f, ki = 0.01f, kd = 0.1f;
    float i_limit = 50.0f, out_limit = 100.0f;
};

struct YoloParam {
    bool        enable     = true;
    std::string param_path = "./best_model.ncnn.param";
    std::string model_path = "./best_model.ncnn.bin";
    float       prob_thresh = 0.25f;
    float       nms_thresh  = 0.45f;
};

/* ─────────────────────── 总配置 ─────────────────────── */

struct CarConfig {
    CameraParam   camera;
    SpeedParam    speed;
    PIDParam      pid_straight;
    PIDParam      pid_curve;
    IPMParam      ipm;
    LineParam     line;
    SceneParam    scene;
    DebugParam    debug;
    SpeedPIDParam speed_pid;
    YoloParam     yolo;
};

/* ─────────────────────── 辅助 ─────────────────────── */

template<typename T>
static T js_get(const json& js, const std::string& key, T def) {
    if (js.contains(key)) {
        try { return js.at(key).get<T>(); }
        catch (...) { std::cerr << "[param] \"" << key << "\" 解析失败\n"; }
    }
    return def;
}

static void parse_pid(PIDParam& p, const json& js) {
    p.kp = js_get(js,"kp",p.kp); p.ki = js_get(js,"ki",p.ki);
    p.kd = js_get(js,"kd",p.kd); p.i_limit = js_get(js,"i_limit",p.i_limit);
    p.out_limit = js_get(js,"out_limit",p.out_limit);
}

/* ─────────────────────── 主加载 ─────────────────────── */

inline bool load_config(CarConfig& cfg, const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.good()) {
        std::cerr << "[param] 找不到 " << path << "，使用默认参数\n";
        return false;
    }
    json js;
    try { ifs >> js; }
    catch (const std::exception& e) {
        std::cerr << "[param] JSON解析失败: " << e.what() << "\n";
        return false;
    }

    if (js.contains("摄像头配置")) {
        auto& j = js["摄像头配置"];
        cfg.camera.img_width  = js_get(j,"img_width",  cfg.camera.img_width);
        cfg.camera.img_height = js_get(j,"img_height", cfg.camera.img_height);
        cfg.camera.fps        = js_get(j,"fps",        cfg.camera.fps);
    }
    if (js.contains("速度配置")) {
        auto& j = js["速度配置"];
        cfg.speed.base_speed     = js_get(j,"base_speed",     cfg.speed.base_speed);
        cfg.speed.speed_curve    = js_get(j,"speed_curve",    cfg.speed.speed_curve);
        cfg.speed.speed_inter    = js_get(j,"speed_inter",    cfg.speed.speed_inter);
        cfg.speed.speed_obstacle = js_get(j,"speed_obstacle", cfg.speed.speed_obstacle);
        cfg.speed.speed_lost     = js_get(j,"speed_lost",     cfg.speed.speed_lost);
    }
    if (js.contains("PID_直道")) parse_pid(cfg.pid_straight, js["PID_直道"]);
    if (js.contains("PID_弯道")) parse_pid(cfg.pid_curve,    js["PID_弯道"]);

    if (js.contains("IPM逆透视")) {
        auto& j = js["IPM逆透视"];
        if (j.contains("src") && j["src"].size()==4)
            for (int k=0;k<4;k++) { cfg.ipm.src[k][0]=j["src"][k][0]; cfg.ipm.src[k][1]=j["src"][k][1]; }
        if (j.contains("dst") && j["dst"].size()==4)
            for (int k=0;k<4;k++) { cfg.ipm.dst[k][0]=j["dst"][k][0]; cfg.ipm.dst[k][1]=j["dst"][k][1]; }
    }
    if (js.contains("巡线参数")) {
        auto& j = js["巡线参数"];
        cfg.line.row_scan_start      = js_get(j,"row_scan_start",      cfg.line.row_scan_start);
        cfg.line.row_scan_end        = js_get(j,"row_scan_end",        cfg.line.row_scan_end);
        cfg.line.road_width_min      = js_get(j,"road_width_min",      cfg.line.road_width_min);
        cfg.line.search_radius       = js_get(j,"search_radius",       cfg.line.search_radius);
        cfg.line.straight_var_thresh = js_get(j,"straight_var_thresh",  cfg.line.straight_var_thresh);
    }
    if (js.contains("场景检测")) {
        auto& j = js["场景检测"];
        cfg.scene.inter_width_thresh   = js_get(j,"inter_width_thresh",   cfg.scene.inter_width_thresh);
        cfg.scene.inter_confirm_frames = js_get(j,"inter_confirm_frames", cfg.scene.inter_confirm_frames);
        cfg.scene.inter_pass_frames    = js_get(j,"inter_pass_frames",    cfg.scene.inter_pass_frames);
        cfg.scene.obstacle_red_ratio   = js_get(j,"obstacle_red_ratio",   cfg.scene.obstacle_red_ratio);
        cfg.scene.obstacle_pass_frames = js_get(j,"obstacle_pass_frames", cfg.scene.obstacle_pass_frames);
        cfg.scene.obstacle_steer       = js_get(j,"obstacle_steer",       cfg.scene.obstacle_steer);
        cfg.scene.zebra_stripe_min     = js_get(j,"zebra_stripe_min",     cfg.scene.zebra_stripe_min);
        cfg.scene.zebra_confirm_frames = js_get(j,"zebra_confirm_frames", cfg.scene.zebra_confirm_frames);
        cfg.scene.zebra_cooldown_frames= js_get(j,"zebra_cooldown_frames",cfg.scene.zebra_cooldown_frames);
        cfg.scene.roundabout_confirm_frames = js_get(j,"roundabout_confirm_frames",cfg.scene.roundabout_confirm_frames);
        cfg.scene.roundabout_entry_frames   = js_get(j,"roundabout_entry_frames",  cfg.scene.roundabout_entry_frames);
        cfg.scene.roundabout_pass_frames    = js_get(j,"roundabout_pass_frames",   cfg.scene.roundabout_pass_frames);
        cfg.scene.roundabout_steer          = js_get(j,"roundabout_steer",         cfg.scene.roundabout_steer);
        cfg.scene.slope_pass_frames = js_get(j,"slope_pass_frames",cfg.scene.slope_pass_frames);
        cfg.scene.speed_slope       = js_get(j,"speed_slope",      cfg.scene.speed_slope);
        cfg.scene.target_bypass_frames = js_get(j,"target_bypass_frames",cfg.scene.target_bypass_frames);
        cfg.scene.target_steer         = js_get(j,"target_steer",        cfg.scene.target_steer);
        cfg.scene.speed_target         = js_get(j,"speed_target",        cfg.scene.speed_target);
    }
    if (js.contains("调试开关")) {
        auto& j = js["调试开关"];
        cfg.debug.debug_stream     = js_get(j,"debug_stream",     cfg.debug.debug_stream);
        cfg.debug.debug_print_fps  = js_get(j,"debug_print_fps",  cfg.debug.debug_print_fps);
        cfg.debug.transmission_port= js_get(j,"transmission_port",cfg.debug.transmission_port);
    }
    if (js.contains("YOLO")) {
        auto& j = js["YOLO"];
        cfg.yolo.enable      = js_get(j,"enable",     cfg.yolo.enable);
        cfg.yolo.param_path  = js_get<std::string>(j,"param_path", cfg.yolo.param_path);
        cfg.yolo.model_path  = js_get<std::string>(j,"model_path", cfg.yolo.model_path);
        cfg.yolo.prob_thresh = js_get(j,"prob_thresh", cfg.yolo.prob_thresh);
        cfg.yolo.nms_thresh  = js_get(j,"nms_thresh", cfg.yolo.nms_thresh);
    }

    std::cout << "[param] 加载成功: " << path << "\n";
    return true;
}
