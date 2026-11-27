#include "scene_detect.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "param/param.hpp"


void scene_state_init(SceneState& state)
{
    memset(&state, 0, sizeof(SceneState));
    state.current               = SCENE_NORMAL;
    state.target_category       = TARGET_NONE;
    state.zebra_pass_count      = 0;
    state.zebra_cooldown_frames = 0;
    state.bypass_error_offset   = 0.0f;
}

//十字路口检测

bool detect_intersection(const LineInfo& line_info, int width_thresh,
                         int row_scan_start)
{
    if (!line_info.found) return false;

    int   count     = 0;
    float width_sum = 0.0f;
    int   row_start = std::min(row_scan_start, IMG_H - 1);

    for (int r = row_start; r >= row_start - 15 && r >= 0; r--) {
        if (line_info.road_width[r] > 0) {
            width_sum += (float)line_info.road_width[r];
            count++;
        }
    }
    if (count == 0) return false;

    float avg = width_sum / (float)count;
    return (avg > (float)width_thresh);
}

//环岛检测

bool detect_roundabout(const LineInfo& line_info, const cv::Mat& bin_ipm,
                       bool& is_left, int row_scan_start)
{
    if (!line_info.found) return false;

    int W = bin_ipm.cols;
    int row_start = std::min(row_scan_start, IMG_H - 1);
    int check_rows = 20;

    int left_lost_cnt  = 0;
    int right_lost_cnt = 0;
    int valid_cnt      = 0;

    for (int r = row_start; r >= row_start - check_rows && r >= 0; r--) {
        if (line_info.road_width[r] <= 0) continue;
        valid_cnt++;
        if (line_info.left_edge[r] <= 2)       left_lost_cnt++;
        if (line_info.right_edge[r] >= W - 3)  right_lost_cnt++;
    }
    if (valid_cnt < 10) return false;

    float left_ratio  = (float)left_lost_cnt  / (float)valid_cnt;
    float right_ratio = (float)right_lost_cnt / (float)valid_cnt;

    int mid_row = std::max(row_start - check_rows / 2, 0);
    float mid_width = 0.0f;
    int   mid_cnt   = 0;
    for (int r = mid_row; r >= mid_row - 5 && r >= 0; r--) {
        if (line_info.road_width[r] > 0) {
            mid_width += (float)line_info.road_width[r];
            mid_cnt++;
        }
    }
    if (mid_cnt > 0) mid_width /= (float)mid_cnt;
    bool width_expand = (mid_width > (float)W * 0.7f);

    const float LOST_THRESH = 0.6f;
    if (left_ratio > LOST_THRESH && right_ratio < 0.2f && width_expand) {
        is_left = true;
        return true;
    }
    if (right_ratio > LOST_THRESH && left_ratio < 0.2f && width_expand) {
        is_left = false;
        return true;
    }
    return false;
}

//红色障碍物检测

bool detect_red_obstacle(const cv::Mat& color_img, float ratio_thresh)
{
    if (color_img.empty()) return false;

    cv::Mat hsv;
    cv::cvtColor(color_img, hsv, cv::COLOR_BGR2HSV);

    int y_start = color_img.rows / 2;
    cv::Mat roi = hsv(cv::Rect(0, y_start, color_img.cols,
                               color_img.rows - y_start));

    cv::Mat mask1, mask2, mask_red;
    cv::inRange(roi, cv::Scalar(  0, 80, 60), cv::Scalar( 10, 255, 255), mask1);
    cv::inRange(roi, cv::Scalar(170, 80, 60), cv::Scalar(180, 255, 255), mask2);
    cv::bitwise_or(mask1, mask2, mask_red);

    int total = roi.rows * roi.cols;
    int red   = cv::countNonZero(mask_red);
    return ((float)red / (float)total > ratio_thresh);
}

//红色矩形检测（目标板标记）

bool detect_red_rectangle(const cv::Mat& color_img, cv::Rect& rect_out)
{
    if (color_img.empty()) return false;

    cv::Mat hsv, mask1, mask2, mask_red;
    cv::cvtColor(color_img, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(  0, 80, 60), cv::Scalar( 10, 255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(170, 80, 60), cv::Scalar(180, 255, 255), mask2);
    cv::bitwise_or(mask1, mask2, mask_red);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask_red, mask_red, cv::MORPH_CLOSE, kernel);
    cv::morphologyEx(mask_red, mask_red, cv::MORPH_OPEN,  kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask_red, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    float best_score = 0.0f;
    bool  found = false;

    for (auto& cnt : contours) {
        cv::Rect br = cv::boundingRect(cnt);
        float area = (float)br.area();
        if (area < 80.0f || area > 3000.0f) continue;

        float aspect = (float)br.width / (float)(br.height + 1);
        if (aspect < 1.2f || aspect > 4.0f) continue;

        float fill = (float)cv::contourArea(cnt) / area;
        if (fill < 0.5f) continue;

        float score = area * fill;
        if (score > best_score) {
            best_score = score;
            rect_out = br;
            found = true;
        }
    }
    return found;
}

//斑马线检测 

bool detect_zebra_line(const cv::Mat& bin_ipm, int stripe_min,
                       int row_scan_start)
{
    if (bin_ipm.empty()) return false;

    int W  = bin_ipm.cols;
    int H  = bin_ipm.rows;
    int cx = W / 2;
    int scan_top    = H * 2 / 3;
    int scan_bottom = std::min(row_scan_start, H - 1);

    int   transitions = 0;
    uchar prev_val    = bin_ipm.at<uchar>(scan_bottom, cx);

    for (int r = scan_bottom - 1; r >= scan_top; r--) {
        uchar cur_val = bin_ipm.at<uchar>(r, cx);
        if ((prev_val >= 128) != (cur_val >= 128)) transitions++;
        prev_val = cur_val;
    }
    return (transitions >= stripe_min * 2);
}

//坡道检测 

bool detect_slope(float accel_x, float accel_y, float accel_z,
                  uint16_t tof_dist_mm)
{
    float pitch_g = fabsf(accel_x);
    bool imu_slope = (pitch_g > 0.15f);
    bool tof_slope = (tof_dist_mm > 0 && tof_dist_mm < 200);
    return imu_slope || tof_slope;
}

//红色区域定位 

static bool locate_red_centroid(const cv::Mat& color_img, bool& on_right)
{
    if (color_img.empty()) return false;

    cv::Mat hsv, mask1, mask2, mask_red;
    cv::cvtColor(color_img, hsv, cv::COLOR_BGR2HSV);
    int y0 = color_img.rows / 2;
    cv::Mat roi = hsv(cv::Rect(0, y0, color_img.cols, color_img.rows - y0));
    cv::inRange(roi, cv::Scalar( 0,80,60), cv::Scalar( 10,255,255), mask1);
    cv::inRange(roi, cv::Scalar(170,80,60),cv::Scalar(180,255,255), mask2);
    cv::bitwise_or(mask1, mask2, mask_red);

    cv::Moments m = cv::moments(mask_red, true);
    if (m.m00 > 0) {
        on_right = (m.m10 / m.m00 > color_img.cols / 2.0);
        return true;
    }
    return false;
}

//综合场景状态机 

SceneType detect_scene(const cv::Mat&   bin_ipm,
                       const cv::Mat&   color_img,
                       const LineInfo&  line_info,
                       SceneState&      state,
                       const SceneParam& sp,
                       int              row_scan_start,
                       float accel_x,
                       float accel_y,
                       float accel_z,
                       uint16_t tof_mm)
{
    /* ═════════════ 持续状态处理（已触发场景，等待退出） ═════════════ */

    /* 十字路口直行保持 */
    if (state.current == SCENE_INTERSECTION) {
        state.inter_pass_frames++;
        if (state.inter_pass_frames >= sp.inter_pass_frames) {
            state.current           = SCENE_NORMAL;
            state.inter_pass_frames = 0;
            state.inter_confirm_cnt = 0;
        }
        return state.current;
    }

    /* 环岛内 */
    if (state.current == SCENE_ROUNDABOUT_IN) {
        state.roundabout_frames++;
        state.bypass_error_offset = 0.0f;  // 环岛内正常巡线
        if (state.roundabout_frames >= sp.roundabout_pass_frames) {
            state.current           = SCENE_NORMAL;
            state.roundabout_frames = 0;
            state.roundabout_in     = false;
        }
        return state.current;
    }

    /* 环岛入口偏转 */
    if (state.current == SCENE_ROUNDABOUT) {
        state.roundabout_frames++;
        // 用偏移量让PID偏向环岛内侧
        float dir = state.roundabout_left ? -1.0f : 1.0f;
        state.bypass_error_offset = dir * (float)sp.roundabout_steer;
        if (state.roundabout_frames >= sp.roundabout_entry_frames) {
            state.current           = SCENE_ROUNDABOUT_IN;
            state.roundabout_frames = 0;
            state.roundabout_in     = true;
            state.bypass_error_offset = 0.0f;
        }
        return state.current;
    }

    /* 红色障碍物绕行（PID + 偏移量，不杀PID） */
    if (state.current == SCENE_OBSTACLE) {
        state.obstacle_pass_frames++;
        // 障碍在左→偏移正值（向右），障碍在右→偏移负值（向左）
        float dir = state.obstacle_side ? -1.0f : 1.0f;
        state.bypass_error_offset = dir * (float)sp.obstacle_steer;
        if (state.obstacle_pass_frames >= sp.obstacle_pass_frames) {
            state.current              = SCENE_NORMAL;
            state.obstacle_pass_frames = 0;
            state.bypass_error_offset  = 0.0f;
        }
        return state.current;
    }

    /* 坡道行驶 */
    if (state.current == SCENE_SLOPE) {
        state.slope_frames++;
        state.bypass_error_offset = 0.0f;
        bool still = detect_slope(accel_x, accel_y, accel_z, tof_mm);
        if (!still || state.slope_frames >= sp.slope_pass_frames) {
            state.current      = SCENE_NORMAL;
            state.slope_frames = 0;
        }
        return state.current;
    }

    /* 斑马线冷却期  */
    if (state.current == SCENE_ZEBRA) {
        state.zebra_cooldown_frames++;
        state.bypass_error_offset = 0.0f;
        if (state.zebra_cooldown_frames >= sp.zebra_cooldown_frames) {
            state.current               = SCENE_NORMAL;
            state.zebra_cooldown_frames = 0;
        }
        return state.current;  // 冷却期内不做任何检测，直接巡线通过
    }

    /* 目标板绕行（PID + 偏移量） */
    if (state.current == SCENE_TARGET_BYPASS) {
        state.target_bypass_frames++;
        // 武器→左绕(负偏移)，物资→右绕(正偏移)，交通→无偏移
        if (state.target_category == TARGET_WEAPON) {
            state.bypass_error_offset = -(float)sp.target_steer;
        } else if (state.target_category == TARGET_SUPPLY) {
            state.bypass_error_offset = (float)sp.target_steer;
        } else {
            state.bypass_error_offset = 0.0f;  // 交通工具直行
        }
        if (state.target_bypass_frames >= sp.target_bypass_frames) {
            state.current              = SCENE_NORMAL;
            state.target_bypass_frames = 0;
            state.target_category      = TARGET_NONE;
            state.target_red_rect_seen = false;
            state.bypass_error_offset  = 0.0f;
        }
        return state.current;
    }

    /* ═════════════ 正常检测流程（按优先级） ═════════════ */

    state.bypass_error_offset = 0.0f;  // 正常模式无偏移

    bool is_target_marker = false;

    /* 优先级1：红色区域检测 */
    if (detect_red_obstacle(color_img, sp.obstacle_red_ratio)) {
        // 区分目标板标记 vs 真路障
        cv::Rect red_rect;
        if (detect_red_rectangle(color_img, red_rect)) {
            float rect_ratio = (float)red_rect.area() /
                               (float)(color_img.rows * color_img.cols);
            int rect_cx = red_rect.x + red_rect.width / 2;
            bool centered = (abs(rect_cx - color_img.cols / 2) < color_img.cols / 4);
            if (rect_ratio < 0.03f && centered) {
                is_target_marker = true;
                state.target_red_rect_seen = true;
            }
        }

        if (!is_target_marker) {
            bool obs_right = false;
            locate_red_centroid(color_img, obs_right);
            state.obstacle_side        = obs_right;
            state.current              = SCENE_OBSTACLE;
            state.obstacle_pass_frames = 0;
            return SCENE_OBSTACLE;
        }
    }

    /* 优先级2：目标板绕行触发 */
    if (state.target_red_rect_seen && state.target_category != TARGET_NONE) {
        state.current              = SCENE_TARGET_BYPASS;
        state.target_bypass_frames = 0;
        return SCENE_TARGET_BYPASS;
    }

    /* 优先级3：坡道 */
    if (line_info.is_straight &&  // ★ 只在直道检测坡道，避免弯道转弯加速度误触发
        detect_slope(accel_x, accel_y, accel_z, tof_mm))
    {
        state.current      = SCENE_SLOPE;
        state.slope_frames = 0;
        return SCENE_SLOPE;
    }

    /* 优先级4：斑马线 */
    if (detect_zebra_line(bin_ipm, sp.zebra_stripe_min, row_scan_start)) {
        state.zebra_confirm_cnt++;
        if (state.zebra_confirm_cnt >= sp.zebra_confirm_frames) {
            state.zebra_confirm_cnt     = 0;
            state.zebra_pass_count++;
            state.current               = SCENE_ZEBRA;
            state.zebra_cooldown_frames = 0;  // 开始冷却
            return SCENE_ZEBRA;
        }
    } else {
        state.zebra_confirm_cnt = 0;
    }

    /* 优先级5：环岛 */
    {
        bool is_left = false;
        if (detect_roundabout(line_info, bin_ipm, is_left, row_scan_start)) {
            state.roundabout_confirm_cnt++;
            if (state.roundabout_confirm_cnt >= sp.roundabout_confirm_frames) {
                state.roundabout_confirm_cnt = 0;
                state.roundabout_left        = is_left;
                state.roundabout_frames      = 0;
                state.current                = SCENE_ROUNDABOUT;
                return SCENE_ROUNDABOUT;
            }
        } else {
            state.roundabout_confirm_cnt = 0;
        }
    }

    /* 优先级6：十字路口 */
    if (detect_intersection(line_info, sp.inter_width_thresh, row_scan_start)) {
        state.inter_confirm_cnt++;
        if (state.inter_confirm_cnt >= sp.inter_confirm_frames) {
            state.inter_confirm_cnt = 0;
            state.current           = SCENE_INTERSECTION;
            state.inter_pass_frames = 0;
            return SCENE_INTERSECTION;
        }
        return state.current;
    } else {
        state.inter_confirm_cnt = 0;
    }

    state.current = SCENE_NORMAL;
    return SCENE_NORMAL;
}
