


// ===================================================================
// 红块检测 + ROI 裁剪：与 PC 端数据集裁剪工具 caijian.cpp(2026-07-11 版)逐行一致
//   - 不粘连：边长 a=底座上沿带红宽,底边压底座上沿向上裁 a×a,附 6% 内容占比校验
//   - 粘连/上方无目标：边长 a=红块宽,底边锁图像最底部向上裁 a×a
//   - 连通域取面积最大者;底座上沿取上边缘拟合线两端更高端
// 车端专属(不改变返回 crop 的算法本身)：
//   - extractTargetByRedBase 附加出参 out_red_mask/out_base_crop/out_bg_color
//   - 红砖判定在"未切除底座"的 base_crop 上进行
//   - verify 第五步 wd_flag 弯道错切(caijian.cpp 无此步)
// ⚠两边任意一侧改动裁剪逻辑,必须同步另一侧
// ===================================================================

#include "red_region_verify.hpp"
#include "black_region_detect.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)
#define LS_RESTRICT __restrict__
#endif


int fa=0;
// ------------------------------------------------------------------
// 保持不变的工具函数
// ------------------------------------------------------------------
static inline bool ls_is_red_custom(unsigned char b, unsigned char g, unsigned char r,
                                    int s_min = 70, int v_min = 40) {
    if (r < v_min || r < g || r < b) return false;
    int minv = (g < b) ? (int)g : (int)b;
    int diff = (int)r - minv;
    if (diff == 0 || diff * 255 < s_min * (int)r) return false;
    if (g >= b) return 30 * ((int)g - (int)b) <= 10 * diff;
    else        return 30 * ((int)b - (int)g) <= 20 * diff;
}

static inline bool br_is_red_hsv_like_bgr(unsigned char b, unsigned char g, unsigned char r) {
    const int RED_S_MIN = 70, RED_V_MIN = 40;
    if (r < RED_V_MIN || r < g || r < b) return false;
    int minv = (g < b) ? (int)g : (int)b;
    int diff = (int)r - minv;
    if (diff == 0 || diff * 255 < RED_S_MIN * (int)r) return false;
    if (g >= b) return 30 * ((int)g - (int)b) <= 10 * diff;
    else        return 30 * ((int)b - (int)g) <= 20 * diff;
}

namespace {
    // 与 PC 端 caijian.cpp 对齐：红掩膜 S 阈值回到 70 并配合 MORPH_OPEN 去噪
    const int   RED_S_MIN            = 70;
    const int   RED_V_MIN            = 40;
    const float ROI_TOP_RATIO        = 0.45f;
    const float MIN_CONTOUR_AREA_RATIO = 0.0005f;
    const float LINE_TOL_PX          = 4.5f;
    const int   MIN_CHAIN_POINTS     = 8;
    const int   BOTTOM_MARGIN_PX     = 5;

    cv::Mat createRedMaskFast(const cv::Mat& img, float top_ratio = ROI_TOP_RATIO) {
        cv::Mat mask = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);
        int roi_top = static_cast<int>(img.rows * top_ratio);
        for (int y = roi_top; y < img.rows; ++y) {
            const cv::Vec3b* src = img.ptr<cv::Vec3b>(y);
            uchar* dst = mask.ptr<uchar>(y);
            for (int x = 0; x < img.cols; ++x)
                if (ls_is_red_custom(src[x][0], src[x][1], src[x][2], RED_S_MIN, RED_V_MIN))
                    dst[x] = 255;
        }
        // 与 PC 端一致：开运算去掉孤立淡红噪点（下游连通域 + 面积门槛再抗噪）
        cv::Mat kernel = cv::Mat::ones(3, 3, CV_8UC1);
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
        return mask;
    }

    float pointLineMaxDistance(const std::vector<cv::Point>& pts) {
        if (pts.size() < 2) return 0.0f;
        cv::Point2f p0(pts.front().x, pts.front().y);
        cv::Point2f p1(pts.back().x,  pts.back().y);
        cv::Point2f v = p1 - p0;
        float den = cv::norm(v);
        if (den < 1e-6f) return 9999.0f;
        float max_dist = 0.0f;
        for (const auto& p : pts) {
            float d = std::abs(v.x * (p.y - p0.y) - v.y * (p.x - p0.x)) / den;
            if (d > max_dist) max_dist = d;
        }
        return max_dist;
    }

    bool fitLineFromChain(const std::vector<cv::Point>& chain, double& angle_deg) {
        if (chain.size() < 2) return false;
        double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
        double min_x = chain[0].x, max_x = chain[0].x;
        int n = (int)chain.size();
        for (const auto& p : chain) {
            sum_x += p.x; sum_y += p.y;
            sum_xy += (double)p.x * p.y; sum_x2 += (double)p.x * p.x;
            if (p.x < min_x) min_x = p.x;
            if (p.x > max_x) max_x = p.x;
        }
        if (max_x - min_x < 2.0) return false;
        double denom = n * sum_x2 - sum_x * sum_x;
        if (std::abs(denom) < 1e-6) return false;
        double k = (n * sum_xy - sum_x * sum_y) / denom;
        double b = (sum_y - k * sum_x) / n;
        double y1 = k * min_x + b;
        double y2 = k * max_x + b;
        angle_deg = std::atan2(y2 - y1, max_x - min_x) * 180.0 / CV_PI;
        return true;
    }

    cv::Vec3b getBgColorMedianFast(const std::vector<cv::Vec3b>& pixels,
                                   int white_thresh = 180, int sat_thresh = 80) {
        if (pixels.empty()) return cv::Vec3b(255, 255, 255);
        std::vector<uchar> b, g, r;
        b.reserve(pixels.size()); g.reserve(pixels.size()); r.reserve(pixels.size());
        for (const auto& p : pixels) {
            uchar cb = p[0], cg = p[1], cr = p[2];
            uchar mx = std::max({cb, cg, cr});
            if (mx >= white_thresh) {
                uchar mn = std::min({cb, cg, cr});
                if ((mx - mn) * 255 <= sat_thresh * mx) {
                    b.push_back(cb); g.push_back(cg); r.push_back(cr);
                }
            }
        }
        if (b.size() < 20) {
            b.clear(); g.clear(); r.clear();
            for (const auto& p : pixels) { b.push_back(p[0]); g.push_back(p[1]); r.push_back(p[2]); }
        }
        auto med = [](std::vector<uchar>& v) -> uchar {
            std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
            return v[v.size() / 2];
        };
        return cv::Vec3b(med(b), med(g), med(r));
    }

    void estimateAndCreateCanvas(const cv::Mat& img, cv::Vec3b& global_bg, cv::Mat& canvas) {
        int h = img.rows, w = img.cols;
        int ch = std::max(3, static_cast<int>(h * 0.10));
        int cw = std::max(3, static_cast<int>(w * 0.10));

        auto get_roi_pixels = [&](cv::Rect r) {
            std::vector<cv::Vec3b> pxs;
            pxs.reserve(r.width * r.height);
            for (int y = r.y; y < r.y + r.height && y < h; ++y) {
                const cv::Vec3b* row_ptr = img.ptr<cv::Vec3b>(y);
                for (int x = r.x; x < r.x + r.width && x < w; ++x)
                    pxs.push_back(row_ptr[x]);
            }
            return pxs;
        };

        cv::Mat corner_img(2, 2, CV_8UC3);
        corner_img.at<cv::Vec3b>(0, 0) = getBgColorMedianFast(get_roi_pixels(cv::Rect(0,     0,     cw, ch)));
        corner_img.at<cv::Vec3b>(0, 1) = getBgColorMedianFast(get_roi_pixels(cv::Rect(w - cw, 0,     cw, ch)));
        corner_img.at<cv::Vec3b>(1, 0) = getBgColorMedianFast(get_roi_pixels(cv::Rect(0,     h - ch, cw, ch)));
        corner_img.at<cv::Vec3b>(1, 1) = getBgColorMedianFast(get_roi_pixels(cv::Rect(w - cw, h - ch, cw, ch)));
        cv::resize(corner_img, canvas, cv::Size(w, h), 0, 0, cv::INTER_LINEAR);

        int edge_h = std::max(3, h / 5);
        std::vector<cv::Vec3b> edge_pxs   = get_roi_pixels(cv::Rect(0, 0,          w, edge_h));
        std::vector<cv::Vec3b> bottom_pxs = get_roi_pixels(cv::Rect(0, h - edge_h, w, edge_h));
        edge_pxs.insert(edge_pxs.end(), bottom_pxs.begin(), bottom_pxs.end());
        global_bg = getBgColorMedianFast(edge_pxs);
    }

    // 大角度旋转后的错切补偿（底部固定，顶部按角度左右平移）——与 PC 端一致，独立一次 warpAffine
    //   shift(y) = top_shift * (1 - y/(h-1))：y=0 顶部平移 top_shift，y=h-1 底部不动。
    //   方向反了把 SHEAR_DIRECTION_SIGN 由 -1 改成 1 即可。
    cv::Mat shearTopKeepBottomByAngle(const cv::Mat& src, double angle_deg) {
        if (src.empty()) return cv::Mat();

        const double SHEAR_TRIGGER_ANGLE_DEG = 20.0;   // 超过此角度才启用错切
        const double SHEAR_GAIN              = 1.2;    // 错切强度：越大顶部平移越多
        const double SHEAR_MAX_SHIFT_RATIO   = 0.55;   // 顶部最大平移不超过图宽的 55%
        const int    SHEAR_DIRECTION_SIGN    = -1;     // 方向反了改成 +1

        int h = src.rows, w = src.cols;
        if (h <= 1 || w <= 1) return src.clone();

        double abs_angle = std::abs(angle_deg);
        if (abs_angle < SHEAR_TRIGGER_ANGLE_DEG) return src.clone();

        double effective_angle = abs_angle - SHEAR_TRIGGER_ANGLE_DEG;
        double top_shift = std::tan(effective_angle * CV_PI / 180.0) * h * SHEAR_GAIN;
        double max_shift = w * SHEAR_MAX_SHIFT_RATIO;
        if (top_shift > max_shift) top_shift = max_shift;
        if (angle_deg < 0) top_shift = -top_shift;
        top_shift *= SHEAR_DIRECTION_SIGN;

        double k = top_shift / static_cast<double>(h - 1);
        cv::Mat M = (cv::Mat_<double>(2, 3) <<
            1.0, -k, top_shift,
            0.0,  1.0, 0.0);
        cv::Mat dst;
        cv::warpAffine(src, dst, M, src.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
        return dst;
    }
}

// ------------------------------------------------------------------
// extractTargetByRedBase
//   返回值       : 去掉底座的“目标 crop”（Target 时的最终输出用）
//   out_red_mask : 整幅扶正红掩膜（底座存在性检查用）
//   out_base_crop: 含底座的底部 a×a crop（红砖连续红行计数用，未去底座）
//   out_bg_color : 估计的背景色（供主函数第五步弯道错切用同色填充，车端独有）
// ------------------------------------------------------------------
static cv::Mat extractTargetByRedBase(const cv::Mat& roi,
                                      cv::Mat* out_red_mask  = nullptr,
                                      cv::Mat* out_base_crop = nullptr,
                                      cv::Vec3b* out_bg_color = nullptr) {
    if (roi.empty()) return cv::Mat();
    int h = roi.rows, w = roi.cols;

    cv::Mat mask = createRedMaskFast(roi);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    if (contours.empty()) return cv::Mat();

    double min_area = std::max(10.0, h * w * (double)MIN_CONTOUR_AREA_RATIO);
    int best_idx = -1; double max_score = -1.0;
    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);
        if (area < min_area) continue;
        int max_y = 0;
        for (auto& p : contours[i]) if (p.y > max_y) max_y = p.y;
        double score = max_y * 100000.0 + area;
        if (score > max_score) { max_score = score; best_idx = (int)i; }
    }
    if (best_idx == -1) return cv::Mat();

    std::vector<cv::Point>& contour = contours[best_idx];
    int c_max_y = 0;
    for (auto& p : contour) if (p.y > c_max_y) c_max_y = p.y;

    std::vector<int> c_idx;
    for (size_t i = 0; i < contour.size(); i++) if (contour[i].y == c_max_y) c_idx.push_back((int)i);
    std::vector<int> xs;
    for (int i : c_idx) xs.push_back(contour[i].x);
    std::sort(xs.begin(), xs.end());
    int median_x = xs[xs.size() / 2];

    int start_idx = c_idx[0];
    int min_diff  = std::abs(contour[c_idx[0]].x - median_x);
    for (int i : c_idx) {
        if (std::abs(contour[i].x - median_x) < min_diff) {
            min_diff = std::abs(contour[i].x - median_x); start_idx = i;
        }
    }

    auto trace_side = [&](int step) {
        std::vector<cv::Point> chain;
        int n = (int)contour.size();
        for (int t = 0; t < n / 2; t++) {
            int idx = (start_idx + step * t % n + n) % n;
            chain.push_back(contour[idx]);
            if ((int)chain.size() < MIN_CHAIN_POINTS) continue;
            if (pointLineMaxDistance(chain) > LINE_TOL_PX) { chain.pop_back(); break; }
        }
        return chain;
    };
    auto chain_a = trace_side(1);
    auto chain_b = trace_side(-1);
    auto calc_len = [](const std::vector<cv::Point>& pts) {
        double t = 0;
        for (size_t i = 1; i < pts.size(); i++) t += cv::norm(pts[i] - pts[i-1]);
        return t;
    };
    auto& chosen = (calc_len(chain_a) >= calc_len(chain_b)) ? chain_a : chain_b;

    double angle_deg = 0;
    if (!fitLineFromChain(chosen, angle_deg)) return cv::Mat();

    cv::Point2f center(w / 2.0f, h / 2.0f);
    cv::Mat rot_M = cv::getRotationMatrix2D(center, angle_deg, 1.0);
    double max_ry = -9999.0;
    for (auto& p : contour) {
        double ny = rot_M.at<double>(1,0)*p.x + rot_M.at<double>(1,1)*p.y + rot_M.at<double>(1,2);
        if (ny > max_ry) max_ry = ny;
    }
    rot_M.at<double>(1,2) += (h - BOTTOM_MARGIN_PX - max_ry);

    cv::Vec3b global_bg; cv::Mat canvas;
    estimateAndCreateCanvas(roi, global_bg, canvas);
    if (out_bg_color) *out_bg_color = global_bg;

    // ====== 扶正：旋转(填背景色) + 有效区羽化合成到背景画布（与 PC 端一致）======
    cv::Mat warped_img, warped_mask;
    cv::warpAffine(roi, warped_img, rot_M, cv::Size(w, h), cv::INTER_LINEAR,
                   cv::BORDER_CONSTANT, cv::Scalar(global_bg[0], global_bg[1], global_bg[2]));
    cv::warpAffine(cv::Mat::ones(h, w, CV_8UC1) * 255, warped_mask, rot_M, cv::Size(w, h),
                   cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));

    int blur_size = std::max(3, std::min(21, static_cast<int>(std::min(h, w) * 0.12f)));
    if (blur_size % 2 == 0) blur_size++;
    cv::GaussianBlur(warped_mask, warped_mask, cv::Size(blur_size, blur_size), 0);

    cv::Mat aligned_img(h, w, CV_8UC3);
    for (int y = 0; y < h; y++) {
        const cv::Vec3b* wp_ptr = warped_img.ptr<cv::Vec3b>(y);
        const cv::Vec3b* cp_ptr = canvas.ptr<cv::Vec3b>(y);
        const uchar* m_ptr = warped_mask.ptr<uchar>(y);
        cv::Vec3b* out_ptr = aligned_img.ptr<cv::Vec3b>(y);
        for (int x = 0; x < w; x++) {
            unsigned alpha = m_ptr[x];
            unsigned inv_alpha = 255u - alpha;
            out_ptr[x][0] = (uchar)((wp_ptr[x][0] * alpha + cp_ptr[x][0] * inv_alpha) >> 8);
            out_ptr[x][1] = (uchar)((wp_ptr[x][1] * alpha + cp_ptr[x][1] * inv_alpha) >> 8);
            out_ptr[x][2] = (uchar)((wp_ptr[x][2] * alpha + cp_ptr[x][2] * inv_alpha) >> 8);
        }
    }

    // ====== 大角度旋转后的错切补偿（底部固定，顶部平移）======
    aligned_img = shearTopKeepBottomByAngle(aligned_img, angle_deg);

    // ====== 红块定位（BFS）======
    cv::Mat roi_mask = createRedMaskFast(aligned_img);
    if (out_red_mask) *out_red_mask = roi_mask.clone();   // 整幅扶正红掩膜（底座存在性检查用）

    int gminx = 99999, gmaxx = -1, gmaxy = -1;
    long red_count = 0;
    for (int y = 0; y < h; ++y) {
        const uchar* mp = roi_mask.ptr<uchar>(y);
        for (int x = 0; x < w; ++x) {
            if (!mp[x]) continue;
            ++red_count;
            if (x < gminx) gminx = x;
            if (x > gmaxx) gmaxx = x;
            gmaxy = y;                 // y 递增，最后命中的红行即最大 y
        }
    }
    if (red_count == 0) return cv::Mat();

    static std::vector<uint8_t>  visited;        // 单线程复用；多线程改 thread_local
    static std::vector<uint16_t> qx, qy;
    visited.assign((size_t)w * h, 0);
    if (qx.size() < (size_t)red_count) { qx.resize(red_count); qy.resize(red_count); }
    std::vector<int> col_top(w, -1), best_col_top(w, -1);
    int best_area = 0, min_x = 0, max_x = -1, bmny = 0, max_y_roi = -1;

    static const int dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const int dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int y = gmaxy; y >= 0; --y) {
        const uchar* mp = roi_mask.ptr<uchar>(y);
        for (int x = gminx; x <= gmaxx; ++x) {
            if (visited[(size_t)y * w + x] || mp[x] == 0) continue;
            int head = 0, tail = 0;
            qx[tail] = (uint16_t)x; qy[tail] = (uint16_t)y; ++tail;
            visited[(size_t)y * w + x] = 1;
            int area = 0, mnx = x, mxx = x, mny = y, mxy = y;
            std::fill(col_top.begin(), col_top.end(), -1);
            while (head < tail) {
                int cx = qx[head], cy = qy[head]; ++head; ++area;
                if (cx < mnx) mnx = cx; if (cx > mxx) mxx = cx;
                if (cy < mny) mny = cy; if (cy > mxy) mxy = cy;
                if (col_top[cx] < 0 || cy < col_top[cx]) col_top[cx] = cy; // 该列最高点(最小y)
                for (int i = 0; i < 8; ++i) {
                    int nx = cx + dx8[i], ny = cy + dy8[i];
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
                    if (visited[(size_t)ny * w + nx]) continue;
                    if (roi_mask.ptr<uchar>(ny)[nx] == 0) continue;
                    visited[(size_t)ny * w + nx] = 1;
                    qx[tail] = (uint16_t)nx; qy[tail] = (uint16_t)ny; ++tail;
                }
            }
            // 取面积最大（并列则更靠下）的块 —— 与 caijian.cpp 一致
            if (area > best_area || (area == best_area && mxy > max_y_roi)) {
                best_area = area; min_x = mnx; max_x = mxx; bmny = mny; max_y_roi = mxy;
                best_col_top = col_top;
            }
        }
    }
    if (best_area <= 0) return cv::Mat();
    (void)bmny;   // caijian.cpp 中同样有此变量但未使用；保留以维持同构

    // ====== 含底座 crop（红砖判定用，未去底座；底部 a×a）======
    if (out_base_crop) {
        int a0  = std::min({std::max(5, max_x - min_x), w, h});
        int cx0 = (min_x + max_x) / 2;
        int x0  = std::max(0, cx0 - a0 / 2);
        int y0  = std::max(0, max_y_roi - a0);
        if (x0 + a0 > w) x0 = w - a0;
        if (y0 + a0 > h) y0 = h - a0;
        *out_base_crop = aligned_img(cv::Rect(x0, y0, a0, a0)).clone();
    }

    // ====== 上边缘拟合直线（允许倾斜）-> 粘连判定 ======
    std::vector<int> ex, ey;
    for (int x = min_x; x <= max_x; ++x)
        if (best_col_top[x] >= 0) { ex.push_back(x); ey.push_back(best_col_top[x]); }
    if (ex.size() < 2) return cv::Mat();

    auto fitLine = [](const std::vector<int>& xs2, const std::vector<int>& ys2, double& k, double& b) {
        int n = (int)xs2.size();
        double sx = 0, sy = 0, sxy = 0, sxx = 0;
        for (int i = 0; i < n; ++i) { sx += xs2[i]; sy += ys2[i]; sxy += (double)xs2[i]*ys2[i]; sxx += (double)xs2[i]*xs2[i]; }
        double den = (double)n * sxx - sx * sx;
        if (std::abs(den) < 1e-6) { k = 0; b = sy / n; }
        else { k = ((double)n * sxy - sx * sy) / den; b = (sy - k * sx) / n; }
    };

    int blk_w = max_x - min_x + 1;
    int TOL = std::max(3, blk_w / 10);
    double k = 0, b = 0;
    fitLine(ex, ey, k, b);
    std::vector<int> ix, iy;
    for (size_t i = 0; i < ex.size(); ++i)
        if (std::abs(ey[i] - (k * ex[i] + b)) <= TOL) { ix.push_back(ex[i]); iy.push_back(ey[i]); }
    if (ix.size() >= 2) fitLine(ix, iy, k, b);

    // 有明显凸起(在拟合直线之上 = top_y 远小于直线)的列数
    int bump = 0;
    for (size_t i = 0; i < ex.size(); ++i)
        if (ey[i] < (k * ex[i] + b) - TOL) ++bump;
    bool stuck = (bump >= std::max(3, blk_w / 8)); // 凸起列多处 => 上边非直线 => 粘连

    // 底座上沿(允许水平倾斜)：取拟合斜线两端更靠上(更小 y)的一端，
    // 保证向上裁时把整条(略微倾斜的)底座都裁掉，不残留红边 —— 与 caijian.cpp 一致
    int yL = (int)std::lround(k * min_x + b);
    int yR = (int)std::lround(k * max_x + b);
    int red_base_top_y = std::min(yL, yR);
    if (red_base_top_y < 5) return cv::Mat();    // 上方几乎没空间
    int base_h = std::max(1, max_y_roi - red_base_top_y);

    if (!stuck) {
        // ===== 正常分开：以底座上沿(留余量)为底边，向上裁 a×a，底座被裁掉 =====
        // 边长 a = 底座上沿带(上沿往下 base_h/4)的红色宽度，横向中心 = 该带中心
        int band = std::max(3, base_h / 4);
        int band_bottom = std::min(max_y_roi, red_base_top_y + band);
        int left = -1, right = -1;
        for (int y = red_base_top_y; y <= band_bottom; ++y) {
            const uchar* mp = roi_mask.ptr<uchar>(y);
            for (int x = min_x; x <= max_x; ++x)
                if (mp[x] > 0) { if (left < 0 || x < left) left = x; if (x > right) right = x; }
        }
        if (right < 0) { left = min_x; right = max_x; }
        int base_margin = std::max(2, red_base_top_y / 40);
        int bottom_y = red_base_top_y - base_margin;
        if (bottom_y >= 5) {
            int a = std::min({std::max(5, right - left + 1), w, h, bottom_y});
            int cx = (left + right) / 2;
            int x1 = cx - a / 2, y1 = bottom_y - a;
            if (x1 < 0) x1 = 0;
            if (x1 + a > w) x1 = w - a;
            if (y1 < 0) y1 = 0;
            // 判断上方裁剪框里是否真有目标：统计框内"非背景"像素占比(阈值很低)。
            // 目标紧贴底座或与底座间有空隙都能命中 => 裁掉底座、保留上方目标；
            // 框里几乎全是背景(红块本身就是目标、上方为空) => 占比不过 => 落到下方兜底裁剪。
            long content = 0, ctot = 0;
            for (int yy = y1; yy < y1 + a; ++yy) {
                const cv::Vec3b* rp = aligned_img.ptr<cv::Vec3b>(yy);
                for (int xx = x1; xx < x1 + a; ++xx) {
                    int dd = std::abs((int)rp[xx][0] - global_bg[0])
                           + std::abs((int)rp[xx][1] - global_bg[1])
                           + std::abs((int)rp[xx][2] - global_bg[2]);
                    if (dd > 40) content++;
                    ctot++;
                }
            }
            if (ctot > 0 && (double)content / ctot >= 0.06)
                return aligned_img(cv::Rect(x1, y1, a, a)).clone();
        }
    }

    // ===== 粘连(或上方空间不足/无目标)：以图像最底部为底边、边长=红块宽度，向上裁 a×a =====
    // 边长=红块宽度，但不超过图像的绝对宽和高；底边严格贴在图像最底部(y = h)向上裁
    int aa = std::min({std::max(5, max_x - min_x + 1), w, h});
    int ccx = (min_x + max_x) / 2;
    int bx = ccx - aa / 2;
    int by = h - aa;                 // 底边锁定图像最底部 —— 与 caijian.cpp"核心修改"一致

    // 防止边界溢出保护
    if (bx < 0) bx = 0;
    if (bx + aa > w) bx = w - aa;
    if (by < 0) by = 0;              // 因为前面限制了 aa <= h，这里通常不会触发，保留兜底

    return aligned_img(cv::Rect(bx, by, aa, aa)).clone();
}

// ------------------------------------------------------------------
// 复用 red_mask 的 5 连续行检查
// ------------------------------------------------------------------
static bool br_has_5_continuous_rows_from_mask(const cv::Mat& red_mask, int img_cols) {
    // 远处检测放宽：原来要求每行红宽 > cols/3、且连续 >=5 行；远处小目标红底座又窄又矮够不到
    // → 必须很近。放宽这两个门槛后远处也能认出底座。
    // 两个可调旋钮(越小越能远，但越容易把远处小红噪点误判成底座 → 误检上升，按实车回调)：
    int       min_red   = std::max(3, img_cols / 8);   // 每行红像素门槛(原 std::max(10, cols/3))
    const int need_rows = 2;                            // 连续行数门槛(原 5)
    int cont = 0;
    for (int y = 0; y < red_mask.rows; ++y) {
        const uchar* row = red_mask.ptr<uchar>(y);
        int cnt = 0;
        for (int x = 0; x < red_mask.cols; ++x) if (row[x]) ++cnt;
        if (cnt > min_red) { if (++cont >= need_rows) return true; }
        else cont = 0;
    }
    return false;
}

// 保留原版（供外部其他路径调用，内部不再使用）
static bool br_has_5_continuous_rows_red_real_hsv(const cv::Mat& roi_bgr) {
    if (roi_bgr.empty()) return false;
    cv::Mat hsv, mask1, mask2, red_mask;
    cv::cvtColor(roi_bgr, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(0, 110, 60),  cv::Scalar(10,  255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(165, 110, 50), cv::Scalar(180, 255, 255), mask2);
    red_mask = mask1 | mask2;
    if (red_mask.rows >= 80 && red_mask.cols >= 80) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
        cv::morphologyEx(red_mask, red_mask, cv::MORPH_OPEN, kernel);
    }
    return br_has_5_continuous_rows_from_mask(red_mask, roi_bgr.cols);
}

// ------------------------------------------------------------------
// 白赛道掩膜(移植自 PC 端 createWhiteTrackMask)
//   亮且低饱和=白;闭运算把白赛道连通;取最大白轮廓填充(内部红底座/物体的洞也被填上)
//   → 得到"赛道面"区域。红块一定在它内部,作为红色检测的空间先验。
// ------------------------------------------------------------------
static cv::Mat createWhiteTrackMask(const cv::Mat& img, int white_min = 140, int sat_max = 45) {
    int h = img.rows, w = img.cols;
    cv::Mat white = cv::Mat::zeros(h, w, CV_8UC1);
    for (int y = 0; y < h; ++y) {
        const cv::Vec3b* sp = img.ptr<cv::Vec3b>(y);
        uchar* wp = white.ptr<uchar>(y);
        for (int x = 0; x < w; ++x) {
            int b = sp[x][0], g = sp[x][1], r = sp[x][2];
            int mx = std::max({b, g, r}), mn = std::min({b, g, r});
            if (mx >= white_min && (mx - mn) <= sat_max) wp[x] = 255;   // 亮+低饱和=白赛道
        }
    }
    int ksz = std::max(3, std::min(h, w) / 25);
    if (ksz % 2 == 0) ksz++;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ksz, ksz));
    cv::morphologyEx(white, white, cv::MORPH_CLOSE, kernel);
    std::vector<std::vector<cv::Point>> wc;
    cv::findContours(white, wc, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::Mat track = cv::Mat::zeros(h, w, CV_8UC1);
    int best = -1; double best_a = 0;
    for (size_t i = 0; i < wc.size(); ++i) {
        double a = cv::contourArea(wc[i]);
        if (a > best_a) { best_a = a; best = (int)i; }
    }
    if (best >= 0) cv::drawContours(track, wc, best, cv::Scalar(255), cv::FILLED);
    return track;
}

// ------------------------------------------------------------------
// 红底座查找(白先验 + 松红 + 贴赛道)：移植自 PC 端 markRedBase 核心，替换原严格 HSV 查找。
//   1) 白赛道掩膜 → 膨胀(让贴赛道边缘的红也算"在赛道上")；
//   2) 全图松红(ls_is_red_custom 把 s_min 放松到 45，远处洗淡的小红块也能收)；
//   3) 红轮廓里"贴赛道 或 足够大(≥4%)"者保留，取最大者为红底座。
//   远处微小红块靠"贴白赛道"这一空间先验被收下，而背景/反光红被赛道外条件挡掉。
//   返回最大红底座的外接框/面积/中心(供粗筛复用,接口与原函数一致)。
// ------------------------------------------------------------------
static RedBlobResult br_find_red_base_on_track(const cv::Mat& roi_bgr) {
    RedBlobResult best{};
    best.min_x = roi_bgr.cols; best.min_y = roi_bgr.rows; best.max_x = -1; best.max_y = -1;
    if (roi_bgr.empty()) return best;
    int h = roi_bgr.rows, w = roi_bgr.cols;

    // 1) 白赛道区域 → 膨胀
    cv::Mat track = createWhiteTrackMask(roi_bgr);
    cv::Mat trackDil;
    int dsz = std::max(3, std::min(h, w) / 16);
    cv::Mat dk = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(dsz, dsz));
    cv::dilate(track, trackDil, dk);

    // 2) 全图松红(s_min 45→30:远处/广角下红底座饱和度被洗淡,45收不到导致 R1a 无红;
    //    finder只做粗筛不在裁剪链上,放松安全;误检由白先验+位置约束+提取兜底)
    cv::Mat redFull = cv::Mat::zeros(h, w, CV_8UC1);
    for (int y = 0; y < h; ++y) {
        const cv::Vec3b* sp = roi_bgr.ptr<cv::Vec3b>(y);
        uchar* mp = redFull.ptr<uchar>(y);
        for (int x = 0; x < w; ++x)
            if (ls_is_red_custom(sp[x][0], sp[x][1], sp[x][2], 30, 40)) mp[x] = 255;
    }

    // 3) 红轮廓:贴赛道 或 足够大 才保留,取最大者
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(redFull, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) return best;
    double big_area = 0.04 * w * h;               // ≥4% 画面的大红块直接收(很近)
    int best_idx = -1; double best_area = 0.0;
    for (size_t i = 0; i < contours.size(); ++i) {
        double a = cv::contourArea(contours[i]);
        bool on_track = false;
        for (const auto& p : contours[i])
            if (trackDil.at<uchar>(p.y, p.x)) { on_track = true; break; }
        if (!on_track && a < big_area) continue;  // 既不贴赛道又不大 → 丢弃
        if (a > best_area) { best_area = a; best_idx = (int)i; }
    }
    if (best_idx < 0) return best;

    cv::Rect bbox = cv::boundingRect(contours[best_idx]);
    best.has_red  = true;
    // 面积用外接框像素数(对几像素的远处小红块比 contourArea 更稳,contourArea 可能≈0)
    best.area     = bbox.width * bbox.height;
    best.min_x    = bbox.x; best.min_y = bbox.y;
    best.max_x    = bbox.x + bbox.width - 1; best.max_y = bbox.y + bbox.height - 1;
    best.width    = bbox.width; best.height = bbox.height;
    best.center_x = bbox.x + bbox.width  / 2;
    best.center_y = bbox.y + bbox.height / 2;
    return best;
}

static bool br_verify_strong_red_in_bbox(const cv::Mat& roi_bgr, const RedBlobResult& blob) {
    if (!blob.has_red) return false;
    int x1 = std::max(0, blob.min_x), y1 = std::max(0, blob.min_y);
    int x2 = std::min(roi_bgr.cols - 1, blob.max_x), y2 = std::min(roi_bgr.rows - 1, blob.max_y);
    int strong = 0, total = 0;
    for (int y = y1; y <= y2; ++y) {
        const cv::Vec3b* row = roi_bgr.ptr<cv::Vec3b>(y);
        for (int x = x1; x <= x2; ++x) {
            uchar b = row[x][0], g = row[x][1], r = row[x][2];
            int mx = std::max((int)g, (int)b);
            ++total;
            if (r >= 90 && r >= mx + 50 && r * 100 >= mx * 180) ++strong;
        }
    }
    return total > 0 && strong * 100 >= blob.area * 30;
}

static bool br_find_red_on_white_track_fast(const cv::Mat& roi_bgr, RedBlobResult& best)
{
    best = {};
    best.min_x = roi_bgr.cols;
    best.min_y = roi_bgr.rows;
    best.max_x = -1;
    best.max_y = -1;

    if (roi_bgr.empty()) return false;

    cv::Mat track = createWhiteTrackMask(roi_bgr);
    if (cv::countNonZero(track) == 0) return false;

    int h = roi_bgr.rows;
    int w = roi_bgr.cols;
    cv::Mat red_mask = cv::Mat::zeros(h, w, CV_8UC1);

    for (int y = 0; y < h; ++y) {
        const uchar* tp = track.ptr<uchar>(y);
        const cv::Vec3b* sp = roi_bgr.ptr<cv::Vec3b>(y);
        uchar* rp = red_mask.ptr<uchar>(y);
        for (int x = 0; x < w; ++x) {
            if (!tp[x]) continue;
            if (ls_is_red_custom(sp[x][0], sp[x][1], sp[x][2], 30, 40)) rp[x] = 255;
        }
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(red_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) return false;

    int best_idx = -1;
    int best_score = -1;
    for (size_t i = 0; i < contours.size(); ++i) {
        cv::Rect rect = cv::boundingRect(contours[i]);
        int area = rect.width * rect.height;
        if (rect.width < 2 || rect.height < 1) continue;
        int score = rect.y * 1000 + rect.height * 100 + area;
        if (score > best_score) {
            best_score = score;
            best_idx = static_cast<int>(i);
        }
    }
    if (best_idx < 0) return false;

    cv::Rect rect = cv::boundingRect(contours[best_idx]);
    best.has_red = true;
    best.area = rect.width * rect.height;
    best.min_x = rect.x;
    best.min_y = rect.y;
    best.max_x = rect.x + rect.width - 1;
    best.max_y = rect.y + rect.height - 1;
    best.width = rect.width;
    best.height = rect.height;
    best.center_x = rect.x + rect.width / 2;
    best.center_y = rect.y + rect.height / 2;
    return true;
}

// ------------------------------------------------------------------
// 主函数
//   流程：粗筛 → 扶正+红块去除 → 底座存在性 → 红砖判定(未去底座) →
//          Target 则输出去底座 crop
// ------------------------------------------------------------------
void verify_and_extract_red_target(
    const cv::Mat& roi,
    int target_y2, int ocx, int ocy, int ox1, int oy1,
    int br_center_x,
    const uint8_t left_border[IMAGE_HEIGHT],
    const uint8_t right_border[IMAGE_HEIGHT])
{
    // ---- 第一步：只在白色赛道中找红块做快速确认 ----
    RedBlobResult red_blob;
    bool red_found = br_find_red_on_white_track_fast(roi, red_blob);

    // 白赛道内没红时，再退回老的 finder 兜底一次，避免个别曝光场景直接漏掉。
    if (!red_found) {
        red_blob = br_find_red_base_on_track(roi);
        red_found = red_blob.has_red && red_blob.width >= 2 && red_blob.height >= 1;
    }

    bool red_ok = red_found;

    if (red_ok) {
        int bx_roi = ocx - ox1;
        int by_roi = ocy - oy1;
        int x_center_tol = (target_y2 < 35) ? (roi.cols / 4) : (roi.cols / 5);
        int x_overlap_tol = roi.cols / 8;
        int y_up_tol = roi.rows / 4;
        int y_down_tol = roi.rows / 3;

        // 黑白图黑块映射到彩色ROI后,红底座必须基本在同一位置附近。
        if (std::abs(red_blob.center_x - bx_roi) > x_center_tol) red_ok = false;
        if (red_ok && (red_blob.max_x < bx_roi - x_overlap_tol || red_blob.min_x > bx_roi + x_overlap_tol)) red_ok = false;
        if (red_ok && (red_blob.center_y < by_roi - y_up_tol || red_blob.center_y > by_roi + y_down_tol)) red_ok = false;
    }

    if (red_ok
        && wd_flag_stable == 0
        && left_halo_flag == 0 && right_halo_flag == 0
        && cross_cnt != 1 && cross_cnt != 2
        && target_y2 >= EARLY_BRAKE_Y2_MIN)
    {
        pre_brake_hint = 1;
    }
    
    if (!red_ok) {
        // [排查] 远距验证失败原因细分:R1a=finder根本没找到红;R1b=找到了但位置约束没过
        // if (target_y2 >= 10 && target_y2 <= 28)
        //     printf("%s y2=%d\n", red_found ? "R1b位置fail" : "R1a无红fail", target_y2);
        Red_flag = 0; Target_flag = 0; Brick_flag = 0; Brick_pos_flag = 0;
        final_crop_img.release(); return;
    }

    // ---- 第二步：扶正 + 红块去除 ----
    //   target_crop      : 去底座的目标 crop（Target 最终输出）
    //   aligned_red_mask : 整幅扶正红掩膜（底座存在性检查）
    //   base_crop        : 含底座 crop（红砖判定，未去底座）
    cv::Mat aligned_red_mask, base_crop;
    cv::Vec3b crop_bg(255, 255, 255);   // 第五步弯道错切的填充色(提取内估计的背景色)
    cv::Mat target_crop = extractTargetByRedBase(roi, &aligned_red_mask, &base_crop, &crop_bg);

    if (target_crop.empty() || base_crop.empty()) {
        // if (target_y2 >= 10 && target_y2 <= 28) printf("R2提取fail y2=%d\n", target_y2);  // [排查]远距验证失败原因
        Red_flag = 0; Target_flag = 0; Brick_flag = 0; Brick_pos_flag = 0;
        final_crop_img.release(); return;
    }

    // ---- 第三步：底座存在性（在“未错切的整幅红掩膜”上做；错切不改变底座是否存在） ----
    if (!br_has_5_continuous_rows_from_mask(aligned_red_mask, base_crop.cols)) {
        // if (target_y2 >= 10 && target_y2 <= 28) printf("R3底座fail y2=%d\n", target_y2);  // [排查]远距验证失败原因
        Red_flag = 0; Target_flag = 0; Brick_flag = 0; Brick_pos_flag = 0;
        final_crop_img.release(); return;
    }
    Red_flag = 1;

    // ---- 第四步：红砖判定（在“未切除底座”的 base_crop 上做，保留底座红行） ----
    int max_cont = 0, cur_cont = 0;
    int row_thresh = base_crop.cols / 4;
    for (int y = 0; y < base_crop.rows; ++y) {
        const uchar* row = base_crop.ptr<uchar>(y);
        int cnt = 0;
        for (int x = 0; x < base_crop.cols; ++x)
            if (br_is_red_hsv_like_bgr(row[x*3], row[x*3+1], row[x*3+2])) ++cnt;
        if (cnt > row_thresh) { if (++cur_cont > max_cont) max_cont = cur_cont; }
        else cur_cont = 0;
    }

    int dyn_thresh = 15 * 4;
    if      (target_y2 < 20)  dyn_thresh = 3 * 4;
    else if (target_y2 <= 30) dyn_thresh = (target_y2 - 10) * 4;
    else if (target_y2 <= 40) dyn_thresh = (target_y2 - 12) * 4;
    else                      dyn_thresh = (target_y2 - 10) * 4;
    if (dyn_thresh > base_crop.rows * 4 / 5) dyn_thresh = base_crop.rows * 4 / 5;
    if (dyn_thresh < 12) dyn_thresh = 12;

    if (max_cont >= dyn_thresh) {
        // ---- 红砖 ----
        Brick_flag = 1; Target_flag = 0;
        int wl = left_border[target_y2], wr = right_border[target_y2];
        int tc = (wl < wr && wl >= 0 && wr < IMAGE_WIDTH) ? (wl + wr) / 2 : 75;
        Brick_pos_flag = (br_center_x < tc) ? 1 : 2;
        final_crop_img.release();
        return;
    }

    // 红色验证已确认“不是红砖、而是卡片”后再应用卡片总开关。
    // 这样 CdOff=1 只屏蔽卡片，不会破坏前面的红砖判定与红砖绕行链。
    if (card_recognition_disable == 1) {
        Red_flag = 0;
        Target_flag = 0;
        Brick_flag = 0;
        Brick_pos_flag = 0;
        pre_brake_hint = 0;
        br_found = 0;
        final_crop_img.release();
        return;
    }

    // ---- 第五步：目标 —— 对“去底座的 target_crop”做弯道补偿后输出 ----
    Brick_flag = 0; Target_flag = 1; Brick_pos_flag = 0;
    if (wd_flag == -1 || wd_flag == 1) {
        float kk = (wd_flag == -1) ? 0.5f : -0.5f;
        float fh = (float)target_crop.rows;
        cv::Mat M = (cv::Mat_<double>(2,3) << 1.0, -(double)kk, (double)(kk*fh), 0.0, 1.0, 0.0);
        cv::Mat sheared;
        // 原为 BORDER_REPLICATE:crop 边缘列(弯道时常是蓝色场地)被复制成约1/4画面的蓝色楔形,
        // 训练集(caijian.cpp 无此步)没有这种分布 → 弯道识别率下降。改为背景色(白系)常量填充。
        cv::warpAffine(target_crop, sheared, M, target_crop.size(),
                       cv::INTER_LINEAR, cv::BORDER_CONSTANT,
                       cv::Scalar(crop_bg[0], crop_bg[1], crop_bg[2]));
        target_crop = sheared;
    }
    final_crop_img = target_crop;
    // capture_save_roi_mjpg("image",fa++);
}

// ==================== [CODEX新增 3/3：红/白掩膜图传调参函数 开始] ====================
void red_mask_tuning_process(CameraStreamServer& server)
{
    // 本函数是独立调试模式：第一次调用时停车并暂停PIT，防止普通菜单同时消费按键。
    // 调试结束后重启程序即可恢复；本函数本身不会修改或保存正式识别参数。
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        car_running = 0;
        Motor_pwm_set(0, 0);
        PWM_FAN_SET(0);
        pit_timer.stop();
        printf("[红掩膜调参] 已启动: KEY1切画面, KEY2选参数, KEY3增加, KEY4减小\n");
    }

    cv::Mat original = uvc_dev.get_frame_mjpg();
    if (original.empty()) return;

    // 最近一次有效的黑块检测 ROI。坐标无效时保留上一帧，避免画面突然清空。
    static cv::Mat latest_roi;
    if (orig_crop_x2 > orig_crop_x1 && orig_crop_y2 > orig_crop_y1) {
        int x1 = std::max(0, std::min(original.cols - 1, orig_crop_x1));
        int y1 = std::max(0, std::min(original.rows - 1, orig_crop_y1));
        int x2 = std::max(0, std::min(original.cols - 1, orig_crop_x2));
        int y2 = std::max(0, std::min(original.rows - 1, orig_crop_y2));
        if (x2 > x1 && y2 > y1)
            latest_roi = original(cv::Rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1)).clone();
    }

    // 独立的松开触发检测。PIT已暂停，因此不会与 menu_isr_handler() 抢按键事件。
    static uint8_t was_down[4] = {0, 0, 0, 0};
    zf_driver_gpio* key_obj[4] = {&key_1, &key_2, &key_3, &key_4};
    bool key_event[4] = {false, false, false, false};
    for (int i = 0; i < 4; ++i) {
        uint8_t level1 = key_obj[i]->get_level();
        uint8_t level2 = key_obj[i]->get_level();
        if (level1 != level2) continue;
        if (level1 == 0) {
            was_down[i] = 1;
        } else if (was_down[i]) {
            was_down[i] = 0;
            key_event[i] = true;
        }
    }

    bool key1_pressed = key_event[0];
    bool key2_pressed = key_event[1];
    bool key3_pressed = key_event[2];
    bool key4_pressed = key_event[3];

    // 所有状态都封装为函数内静态变量，不影响项目原有全局参数。
    static int view = 0;              // 0原图 1原ROI 2白掩膜 3红掩膜
    static int selected_param = 0;    // 0=S_MIN 1=V_MIN
    static int s_min = 70;
    static int v_min = 40;
    const char* view_name[4] = {"原始彩色图", "原始ROI", "白色掩膜", "红色掩膜"};

    // -------------------- 调试模式下的 NCNN 单次识别 --------------------
    // 正式状态机现在要求“满足输入条件→强制停车→编码器确认停稳→NCNN”；
    // 本调试函数首次运行就停车并停止PIT，因此这里只保留卡片和输入图像有效性条件。
    static bool ncnn_done_for_current_target = false;
    static int target_missing_frames = 0;

    if (Target_flag == 0) {
        // 连续丢失5帧后才认为上一张卡已离开，避免检测闪断造成同一卡片重复推理。
        if (target_missing_frames < 5) target_missing_frames++;
        if (target_missing_frames >= 5) ncnn_done_for_current_target = false;
    } else {
        target_missing_frames = 0;
    }

    bool ncnn_condition =
        (Target_flag == 1) &&
        (recog_staute == 0) &&
        (target_y2 > DETECT_START_DIS) &&
        g_ncnn_inited &&
        !final_crop_img.empty() &&
        (final_crop_img.cols >= 20) &&
        (final_crop_img.rows >= 20);

    if (ncnn_condition && !ncnn_done_for_current_target) {
        // 先清空上一次结果，防止模型入口异常返回时误用旧分类。
        single_out = -1;
        single_out_prob = 0.0f;
        ncnn_recog_run_roi_float();
        ncnn_done_for_current_target = true;

        if (single_out >= 0 && single_out < NUM_CLASSES) {
            printf("[红掩膜调参][NCNN] 识别结果=%s, class=%d, prob=%.4f\n",
                   class_labels[single_out], single_out, single_out_prob);
        } else {
            printf("[红掩膜调参][NCNN] 推理失败或没有有效输出\n");
        }
    }

    // -------------------- 每10帧输出一次卡片/NCNN门槛诊断 --------------------
    // 这里只读取现有状态，不改正式检测结果。诊断顺序与主流程的门控顺序一致。
    static uint32_t card_debug_frame_count = 0;
    card_debug_frame_count++;
    if (card_debug_frame_count >= 10) {
        card_debug_frame_count = 0;

        int crop_w = final_crop_img.empty() ? 0 : final_crop_img.cols;
        int crop_h = final_crop_img.empty() ? 0 : final_crop_img.rows;
        bool raw_roi_valid =
            (orig_crop_x2 > orig_crop_x1) &&
            (orig_crop_y2 > orig_crop_y1);

        if (Target_flag != 1) {
            if (banmaxian_flag == 1) {
                printf("[卡片诊断] 未作为卡片: 当前为斑马线状态，main跳过黑色卡片检测\n");
            }
            else if (evade_cooldown_active == 1) {
                printf("[卡片诊断] 未作为卡片: 绕行冷却锁生效，main跳过黑色卡片检测\n");
            }
            else if (brick_dis_isp == 1) {
                printf("[卡片诊断] 未作为卡片: 正在执行红砖避障，卡片判定被冻结\n");
            }
            else if (Brick_flag == 1) {
                printf("[卡片诊断] 未作为卡片: 当前目标被判定为红砖 (Brick_pos=%d)\n",
                       Brick_pos_flag);
            }
            else if (br_found == 0) {
                printf("[卡片诊断] 未作为卡片: 二值图中未找到满足条件的黑色卡片候选"
                       " (y2=%d, end=%d, wd=%d, halo=%d/%d)\n",
                       target_y2, min_efect_row_end, wd_flag_stable,
                       left_halo_flag, right_halo_flag);
            }
            else if (!raw_roi_valid) {
                printf("[卡片诊断] 未作为卡片: 已找到黑块，但原图ROI坐标无效"
                       " (%d,%d)-(%d,%d)\n",
                       orig_crop_x1, orig_crop_y1, orig_crop_x2, orig_crop_y2);
            }
            else if (Red_flag == 0) {
                // 每10帧仅重跑一次红色验证用于定位，不写回任何正式检测标志。
                RedBlobResult diag_blob;
                bool diag_red_found = br_find_red_on_white_track_fast(latest_roi, diag_blob);
                if (!diag_red_found) {
                    diag_blob = br_find_red_base_on_track(latest_roi);
                    diag_red_found = diag_blob.has_red && diag_blob.width >= 2 && diag_blob.height >= 1;
                }

                if (!diag_red_found) {
                    printf("[卡片诊断] 未作为卡片: R1a，彩色ROI中没有找到满足条件的红底座"
                           " (检查白色掩膜、红色阈值和曝光)\n");
                } else {
                    int diag_ocx = static_cast<int>(br_center_x * (320.0f / 150.0f) + 0.5f);
                    int diag_ocy = static_cast<int>(br_center_y * (240.0f / 60.0f) + 0.5f);
                    int bx_roi = diag_ocx - orig_crop_x1;
                    int by_roi = diag_ocy - orig_crop_y1;
                    int x_center_tol = (target_y2 < 35) ? (latest_roi.cols / 4) : (latest_roi.cols / 5);
                    int x_overlap_tol = latest_roi.cols / 8;
                    int y_up_tol = latest_roi.rows / 4;
                    int y_down_tol = latest_roi.rows / 3;
                    bool diag_position_ok = true;
                    if (std::abs(diag_blob.center_x - bx_roi) > x_center_tol) diag_position_ok = false;
                    if (diag_position_ok &&
                        (diag_blob.max_x < bx_roi - x_overlap_tol ||
                         diag_blob.min_x > bx_roi + x_overlap_tol)) diag_position_ok = false;
                    if (diag_position_ok &&
                        (diag_blob.center_y < by_roi - y_up_tol ||
                         diag_blob.center_y > by_roi + y_down_tol)) diag_position_ok = false;

                    if (!diag_position_ok) {
                        printf("[卡片诊断] 未作为卡片: R1b，找到红色但与二值黑块位置不匹配"
                               " | red=(%d,%d), blackROI=(%d,%d), tolX=%d\n",
                               diag_blob.center_x, diag_blob.center_y,
                               bx_roi, by_roi, x_center_tol);
                    } else {
                        cv::Mat diag_red_mask, diag_base_crop;
                        cv::Vec3b diag_bg(255, 255, 255);
                        cv::Mat diag_target_crop = extractTargetByRedBase(
                            latest_roi, &diag_red_mask, &diag_base_crop, &diag_bg);

                        if (diag_target_crop.empty() || diag_base_crop.empty()) {
                            printf("[卡片诊断] 未作为卡片: R2，红底座存在，但目标扶正/裁剪失败\n");
                        }
                        else if (!br_has_5_continuous_rows_from_mask(
                                     diag_red_mask, diag_base_crop.cols)) {
                            printf("[卡片诊断] 未作为卡片: R3，目标已裁出，但红底座连续红行门槛未通过\n");
                        }
                        else {
                            printf("[卡片诊断] 红色验证复查已全部通过，但正式Target_flag仍为0"
                                   "，请检查同一帧内状态是否被后续逻辑覆盖\n");
                        }
                    }
                }
            }
            else if (final_crop_img.empty()) {
                printf("[卡片诊断] 未作为卡片: 红底座已通过，但最终NCNN卡片裁剪为空\n");
            }
            else {
                printf("[卡片诊断] 未作为卡片: Target_flag未置1，现有状态无法进一步区分"
                       " | br=%d red=%d brick=%d crop=%dx%d y2=%d\n",
                       br_found, Red_flag, Brick_flag, crop_w, crop_h, target_y2);
            }
        }
        else if (!ncnn_done_for_current_target) {
            if (recog_staute != 0) {
                printf("[卡片诊断] 已是卡片但未启动NCNN: recog_staute=%d，识别/绕行状态未空闲\n",
                       recog_staute);
            }
            else if (target_y2 <= DETECT_START_DIS) {
                printf("[卡片诊断] 已是卡片但未启动NCNN: 距离未到"
                       " (target_y2=%d <= DETECT_START_DIS=%d)\n",
                       target_y2, DETECT_START_DIS);
            }
            else if (!g_ncnn_inited) {
                printf("[卡片诊断] 已是卡片但未启动NCNN: 模型未初始化成功\n");
            }
            else if (final_crop_img.empty()) {
                printf("[卡片诊断] 已是卡片但未启动NCNN: final_crop_img为空\n");
            }
            else if (crop_w < 20 || crop_h < 20) {
                printf("[卡片诊断] 已是卡片但未启动NCNN: 裁剪尺寸过小"
                       " (%dx%d，要求至少20x20)\n", crop_w, crop_h);
            }
            else {
                printf("[卡片诊断] 已满足NCNN条件，等待本帧调用"
                       " | y2=%d/%d crop=%dx%d\n",
                       target_y2, DETECT_START_DIS, crop_w, crop_h);
            }
        }
        else {
            printf("[卡片诊断] 已完成本卡片NCNN单次识别"
                   " | class=%d prob=%.4f target_y2=%d crop=%dx%d\n",
                   single_out, single_out_prob, target_y2, crop_w, crop_h);
        }
    }

    if (key1_pressed) {
        view = (view + 1) % 4;
        printf("[红掩膜调参] 切换画面: %s\n", view_name[view]);
    }

    if (view == 3) {
        if (key2_pressed) {
            selected_param = !selected_param;
            printf("[红掩膜调参] 当前调节参数: %s (S_MIN=%d, V_MIN=%d)\n",
                   selected_param == 0 ? "S_MIN" : "V_MIN", s_min, v_min);
        }

        int delta = 0;
        if (key3_pressed) delta += 1;
        if (key4_pressed) delta -= 1;

        if (delta != 0) {
            int* value = (selected_param == 0) ? &s_min : &v_min;
            *value = std::max(0, std::min(255, *value + delta));
            printf("[红掩膜调参] S_MIN=%d, V_MIN=%d (当前=%s)\n",
                   s_min, v_min, selected_param == 0 ? "S_MIN" : "V_MIN");
        }
    }

    cv::Mat display;
    switch (view) {
        case 0:
            display = original;
            break;

        case 1:
            display = latest_roi;
            break;

        case 2:
            if (!latest_roi.empty()) display = createWhiteTrackMask(latest_roi);
            break;

        case 3:
            if (!latest_roi.empty()) {
                display = cv::Mat::zeros(latest_roi.rows, latest_roi.cols, CV_8UC1);
                int roi_top = static_cast<int>(latest_roi.rows * 0.45f);
                for (int y = roi_top; y < latest_roi.rows; ++y) {
                    const cv::Vec3b* src = latest_roi.ptr<cv::Vec3b>(y);
                    uchar* dst = display.ptr<uchar>(y);
                    for (int x = 0; x < latest_roi.cols; ++x) {
                        if (ls_is_red_custom(src[x][0], src[x][1], src[x][2], s_min, v_min))
                            dst[x] = 255;
                    }
                }
                cv::Mat kernel = cv::Mat::ones(3, 3, CV_8UC1);
                cv::morphologyEx(display, display, cv::MORPH_OPEN, kernel);
            }
            break;

        default:
            display = original;
            break;
    }

    if (display.empty()) {
        display = cv::Mat::zeros(original.size(), CV_8UC3);
        cv::putText(display, "WAITING FOR CARD ROI", cv::Point(22, original.rows / 2),
                    cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 255, 255), 2);
    }

    server.update_frame(display);
}
// ==================== [CODEX新增 3/3：红/白掩膜图传调参函数 结束] ====================
