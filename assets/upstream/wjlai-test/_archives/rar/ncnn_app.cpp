#include "ncnn_app.hpp"
#include <cstdio>
#include <vector>



// ============================================================================
// 红色发车挡板检测算法 (极简高效版)
// ============================================================================
bool CheckRedBaffle(const cv::Mat& frame)
{
    if (frame.empty()) return false;

    // 1. 视场裁剪：砍掉上半部分，彻底无视远处的任何红色干扰
    cv::Rect roi(0, frame.rows / 2, frame.cols, frame.rows / 2);
    cv::Mat bottom_frame = frame(roi);

    cv::Mat hsv, red_mask;

    cv::cvtColor(bottom_frame, hsv, cv::COLOR_BGR2HSV);

    // 2. 颜色防线
    cv::Mat mask1, mask2;
    cv::inRange(hsv, cv::Scalar(0, 60, 55), cv::Scalar(10, 255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(170, 60, 55), cv::Scalar(180, 255, 255), mask2);
    red_mask = mask1 | mask2;
    
    //cv::Mat hsv, green_mask;
    //cv::cvtColor(bottom_frame, hsv, cv::COLOR_BGR2HSV);
    //cv::inRange(hsv, cv::Scalar(40, 60, 55), cv::Scalar(80, 255, 255), green_mask); //绿色挡板


    // 3. 寻找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(red_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    if (contours.empty()) return false;

    // 4. 找到最大红块面积
    double max_area = 0;
    for (const auto& c : contours) {
        double area = cv::contourArea(c);
        if (area > max_area) {
            max_area = area;
        }
    }

    // 5. 面积判断：设定阈值 300
    if (max_area > 300) {
        return true;  // 检测到挡板！
    }

    return false;     // 没有挡板
}
