#include "line_detect.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>


static inline int clamp_x(int x, int W)
{
    if (x < 0)   return 0;
    if (x >= W)  return W - 1;
    return x;
}

static int search_left(const uchar* row_ptr, int start_x, int W)
{
    int x = clamp_x(start_x, W);
    while (x > 0 && row_ptr[x] >= 128) {
        x--;
    }
    return x;
}

static int search_right(const uchar* row_ptr, int start_x, int W)
{
    int x = clamp_x(start_x, W);
    while (x < W - 1 && row_ptr[x] >= 128) {
        x++;
    }
    return x;
}

static int find_road_near(const uchar* row_ptr, int hint_x, int radius, int W)
{
    for (int d = 0; d < radius; d++) {
        int xl = hint_x - d;
        int xr = hint_x + d;
        if (xl >= 0   && row_ptr[xl] >= 128) return xl;
        if (xr < W    && row_ptr[xr] >= 128) return xr;
    }
    return -1;
}


void line_info_reset(LineInfo& info)
{
    memset(&info, 0, sizeof(LineInfo));
    for (int r = 0; r < IMG_H; r++) {
        info.left_edge [r] = IMG_W / 2;
        info.right_edge[r] = IMG_W / 2;
        info.center_x  [r] = IMG_W / 2;
        info.road_width[r] = 0;
    }
    info.error       = 0.0f;
    info.avg_width   = 0.0f;
    info.valid_rows  = 0;
    info.top_row     = IMG_H - 1;
    info.found       = false;
    info.left_lost   = false;
    info.right_lost  = false;
    info.is_straight = false;
}

//迷宫法巡线
void maze_detect(const cv::Mat& bin_ipm, LineInfo& info, const LineParam& lp)
{
    int W = bin_ipm.cols;
    int H = bin_ipm.rows;

    if (bin_ipm.empty() || bin_ipm.type() != CV_8UC1) {
        std::cerr << "[line_detect] 输入图像无效\n";
        return;
    }

    int ROW_START  = std::min(lp.row_scan_start, H - 1);
    int ROW_END    = std::max(lp.row_scan_end,   0);
    int WIDTH_MIN  = lp.road_width_min;
    int RADIUS     = lp.search_radius;
    int CX         = W / 2;

    int seed_x = CX;
    {
        const uchar* bottom = bin_ipm.ptr<uchar>(ROW_START);
        int fx = find_road_near(bottom, seed_x, RADIUS, W);
        if (fx >= 0) seed_x = fx;
    }

    float  width_sum  = 0.0f;
    float  err_sum    = 0.0f;
    float  weight_sum = 0.0f;
    int    top_row_tmp = ROW_START;
    int    miss_streak = 0;

    for (int r = ROW_START; r >= ROW_END; r--)
    {
        const uchar* row = bin_ipm.ptr<uchar>(r);

        if (row[clamp_x(seed_x, W)] < 128) {
            int fx = find_road_near(row, seed_x, RADIUS, W);
            if (fx < 0) {
                miss_streak++;
                if (miss_streak > 5) break;
                if (r < H - 1) {
                    info.left_edge [r] = info.left_edge [r + 1];
                    info.right_edge[r] = info.right_edge[r + 1];
                    info.center_x  [r] = info.center_x  [r + 1];
                    info.road_width[r] = 0;
                }
                continue;
            }
            seed_x = fx;
        }
        miss_streak = 0;

        int lx = search_left (row, seed_x, W);
        int rx = search_right(row, seed_x, W);

        int width  = rx - lx;
        int center = (lx + rx) / 2;

        if (width < WIDTH_MIN) {
            if (r < H - 1) {
                info.left_edge [r] = info.left_edge [r + 1];
                info.right_edge[r] = info.right_edge[r + 1];
                info.center_x  [r] = info.center_x  [r + 1];
                info.road_width[r] = 0;
            }
            continue;
        }

        info.left_edge [r] = lx;
        info.right_edge[r] = rx;
        info.center_x  [r] = center;
        info.road_width[r] = width;
        info.valid_rows++;
        top_row_tmp = r;
        info.found   = true;

        seed_x = center;

        width_sum += (float)width;

        float w = (float)(r - ROW_END) / (float)(ROW_START - ROW_END + 1);
        w = w * w;
        err_sum    += w * (float)(center - CX);
        weight_sum += w;
    }

    if (info.found && weight_sum > 0.0f) {
        info.error    = err_sum / weight_sum;
        info.avg_width = width_sum / (float)info.valid_rows;
        info.top_row  = top_row_tmp;

        int bot_r = ROW_START;
        info.left_lost  = (info.left_edge [bot_r] <= 2);
        info.right_lost = (info.right_edge[bot_r] >= W - 3);
    }
}

//直行
void detect_straight(LineInfo& info, const LineParam& lp, int check_rows)
{
    if (!info.found) {
        info.is_straight = false;
        return;
    }

    int ROW_START = std::min(lp.row_scan_start, IMG_H - 1);

    float sum    = 0.0f;
    float sq_sum = 0.0f;
    int   cnt    = 0;

    for (int r = ROW_START;
         r > ROW_START - check_rows && r >= lp.row_scan_end;
         r--)
    {
        if (info.road_width[r] > 0) {
            float cx = (float)info.center_x[r];
            sum    += cx;
            sq_sum += cx * cx;
            cnt++;
        }
    }

    if (cnt < 3) {
        info.is_straight = false;
        return;
    }

    float mean     = sum / (float)cnt;
    float variance = sq_sum / (float)cnt - mean * mean;
    float std_dev  = sqrtf(variance > 0.0f ? variance : 0.0f);

    info.is_straight = (std_dev < lp.straight_var_thresh);
}

//在摄像头中可视化巡线偏差
void draw_line_result(cv::Mat& img, const LineInfo& info, const LineParam& lp)
{
    if (!info.found) return;

    cv::Mat disp;
    if (img.channels() == 1) {
        cv::cvtColor(img, disp, cv::COLOR_GRAY2BGR);
        img = disp;
    }

    int ROW_END   = lp.row_scan_end;
    int ROW_START = lp.row_scan_start;

    for (int r = ROW_END; r <= ROW_START; r++) {
        if (info.road_width[r] <= 0) continue;

        cv::circle(img, cv::Point(info.left_edge [r], r), 1,
                   cv::Scalar(255, 80, 80), -1);
        cv::circle(img, cv::Point(info.right_edge[r], r), 1,
                   cv::Scalar(80, 80, 255), -1);
        cv::circle(img, cv::Point(info.center_x  [r], r), 1,
                   cv::Scalar(80, 255, 80), -1);
    }

    cv::line(img,
             cv::Point(IMG_W / 2, ROW_END),
             cv::Point(IMG_W / 2, ROW_START),
             cv::Scalar(0, 220, 220), 1);

    char buf[64];
    snprintf(buf, sizeof(buf), "err:%.1f %s w:%.0f",
             info.error,
             info.is_straight ? "ST" : "CV",
             info.avg_width);
    cv::putText(img, buf,
                cv::Point(2, 12),
                cv::FONT_HERSHEY_PLAIN, 0.8,
                cv::Scalar(0, 255, 255), 1);
}
