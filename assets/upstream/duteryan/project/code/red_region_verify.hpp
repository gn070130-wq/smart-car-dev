

#ifndef __RED_REGION_VERIFY_HPP__
#define __RED_REGION_VERIFY_HPP__

#include "zf_common_headfile.hpp"
#include <opencv2/opencv.hpp>

// ==================== [CODEX新增 1/3：图传类前置声明 开始] ====================
class CameraStreamServer;
// ==================== [CODEX新增 1/3：图传类前置声明 结束] ====================

// ==========================================================
// 结构体声明：用于存储红色特征扫描与连通域提取的结果
// ==========================================================

// 快速扫描红色的结果结构体 (老版本备用)
struct LsFastRedResult {
    bool has_red;                   // 是否含有红色
    bool early_brick;               // 是否提前判定为红砖
    int max_continuous_red_rows;    // 最大连续包含红色的行数
    int min_x;                      // 红色区域最左侧坐标
    int max_x;                      // 红色区域最右侧坐标
    int max_y;                      // 红色区域最下方坐标
};

// 存储红色连通域特征的结果结构体 (现役核心)
struct RedBlobResult
{
    bool has_red;      // 是否找到红色连通域
    int area;          // 最大红色连通域面积
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    int center_x;
    int center_y;
    int width;
    int height;
};

// ==========================================================
// 核心验证接口声明
// ==========================================================

/**
 * @brief 执行高分辨率 ROI 内的红色区域验证、分类与截取
 * @param roi           通过摄像头截取出的高清彩色 ROI 图像
 * @param target_y2     目标在 150x60 低分辨率下的最下沿 Y 坐标
 * @param ocx           黑块映射到 320x240 原图的中心 X 坐标
 * @param ocy           黑块映射到 320x240 原图的中心 Y 坐标
 * @param ox1           ROI 在 320x240 原图中的左上角 X 坐标
 * @param oy1           ROI 在 320x240 原图中的左上角 Y 坐标
 * @param br_center_x   低分辨率下黑块的 X 坐标 (用于判定方位)
 * @param left_border   赛道左边界数组
 * @param right_border  赛道右边界数组
 */
void verify_and_extract_red_target(
    const cv::Mat& roi,
    int target_y2, 
    int ocx, 
    int ocy, 
    int ox1, 
    int oy1, 
    int br_center_x,
    const uint8_t left_border[IMAGE_HEIGHT],
    const uint8_t right_border[IMAGE_HEIGHT]
);

// ==================== [CODEX新增 2/3：调参函数声明 开始] ====================
/**
 * @brief 红色掩膜实时调参处理（每个摄像头新帧调用一次）
 *
 * 首次调用后进入调参模式并独占停车状态下的按键：
 *   KEY1：原始彩色图 -> 原始ROI -> 白色掩膜 -> 红色掩膜 -> 循环
 *   KEY2：在红色掩膜页切换 S_MIN / V_MIN
 *   KEY3：当前参数 +1
 *   KEY4：当前参数 -1
 *
 * @param server 已启动的 ww_camera_server 图传对象
 */
void red_mask_tuning_process(CameraStreamServer& server);
// ==================== [CODEX新增 2/3：调参函数声明 结束] ====================

#endif // __RED_REGION_VERIFY_HPP__
