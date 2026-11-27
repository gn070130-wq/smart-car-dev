#pragma once
#include <opencv2/opencv.hpp>
#include "line_detect.h"

/* ═══════════════════════════ 场景枚举 ════════════════════════════════ */

enum SceneType {
    SCENE_NORMAL = 0,
    SCENE_INTERSECTION,       // 十字路口（直行保持）
    SCENE_ROUNDABOUT,         // 环岛入口（偏转进入）
    SCENE_ROUNDABOUT_IN,      // 环岛内（巡线行驶）
    SCENE_OBSTACLE,           // 红色路障（偏移绕行）
    SCENE_SLOPE,              // 坡道（减速巡线）
    SCENE_ZEBRA,              // 斑马线（冷却通过 / 终点停车）
    SCENE_TARGET_BOARD,       // 目标板（等待识别）
    SCENE_TARGET_BYPASS,      // 目标板绕行（偏移巡线）
};

/* ═══════════════════════════ 目标板分类 ════════════════════════════════ */

enum TargetCategory {
    TARGET_NONE = 0,
    TARGET_WEAPON,            // 武器 → 左侧绕行
    TARGET_SUPPLY,            // 物资 → 右侧绕行
    TARGET_VEHICLE,           // 交通工具 → 直行压过
};

/* ═══════════════════════════ 前向声明 ═════════════════════════════════ */

struct SceneParam;

/* ═══════════════════════════ 场景状态机 ═══════════════════════════════ */

struct SceneState {
    SceneType current;

    /* 十字路口 */
    int  inter_confirm_cnt;
    int  inter_pass_frames;

    /* 环岛 */
    int  roundabout_confirm_cnt;
    int  roundabout_frames;
    bool roundabout_in;
    bool roundabout_left;

    /* 障碍物 */
    bool obstacle_side;           // false=左, true=右
    int  obstacle_pass_frames;

    /* 坡道 */
    int  slope_frames;

    /* 目标板 */
    TargetCategory target_category;
    int  target_confirm_cnt;
    int  target_bypass_frames;
    bool target_red_rect_seen;

    /* ★ 斑马线 — 新增冷却期 */
    int  zebra_confirm_cnt;
    int  zebra_pass_count;        // 第几次经过（1=发车，2=终点）
    int  zebra_cooldown_frames;   // 冷却计数器（通过时不重复计数）

    /* ★ 绕行偏移量（给PID加offset，而非固定转向） */
    float bypass_error_offset;    // 当前叠加到 PID error 上的偏移像素
};

/* ═══════════════════════════ 函数声明 ═════════════════════════════════ */

void scene_state_init(SceneState& state);

bool detect_intersection(const LineInfo& line_info, int width_thresh, int row_scan_start);
bool detect_red_obstacle(const cv::Mat& color_img, float ratio_thresh);
bool detect_zebra_line(const cv::Mat& bin_ipm, int stripe_min, int row_scan_start);
bool detect_slope(float accel_x, float accel_y, float accel_z, uint16_t tof_dist_mm);
bool detect_roundabout(const LineInfo& line_info, const cv::Mat& bin_ipm,
                       bool& is_left, int row_scan_start);
bool detect_red_rectangle(const cv::Mat& color_img, cv::Rect& rect_out);

SceneType detect_scene(const cv::Mat&   bin_ipm,
                       const cv::Mat&   color_img,
                       const LineInfo&  line_info,
                       SceneState&      state,
                       const SceneParam& sp,
                       int              row_scan_start,
                       float accel_x  = 0.0f,
                       float accel_y  = 0.0f,
                       float accel_z  = 1.0f,
                       uint16_t tof_mm = 8192);
