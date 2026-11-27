#ifndef __VISION_HPP__
#define __VISION_HPP__

// 这一版视觉专门用于“白底赛道、两边为蓝色”的巡线。
// 当前算法的核心思路是：
// 1. 先用图像底部若干行估计本帧白赛道亮度；
// 2. 再从整幅图里找到“最远还能保持白色”的参考列；
// 3. 以该参考列为起点，每一行向左右扫边界；
// 4. 最后根据左右边界计算中线和误差，提供给底盘控制。
#include "zf_common_headfile.hpp"

#define SEARCH_UVC_WIDTH             UVC_WIDTH     // 参与视觉处理的图像宽度
#define SEARCH_UVC_HEIGHT            UVC_HEIGHT    // 参与视觉处理的图像高度
#define BLACKPOINT                   50            // 动态阈值计算时允许的最低灰度，避免阈值太低
#define WHITEMAXMUL                  11            // 白阈值上限倍率，最终会除以 10
#define WHITEMINMUL                  7             // 白阈值下限倍率，最终会除以 10
#define REFERENCE_ROW                5             // 用底部多少行来估计“白赛道参考亮度”
#define REFERENCE_CENTER_WIDTH       10            // 只取图像中间多少列参与底部亮度参考，避免两侧赛道外区域干扰
#define CONTRASTOFFSET               1             // 行列采样步长，不逐像素扫，减小计算量
#define SEARCHSTOPROW                0             // 向上搜索允许到达的最高行
#define SEARCH_COL_COUNT             ((SEARCH_UVC_WIDTH + CONTRASTOFFSET - 1) / CONTRASTOFFSET)

// 红色标识纸颜色与几何筛选参数，尺寸参数以 160x120 检测帧为准。
#define RED_HUE_LOW_MIN              0
#define RED_HUE_LOW_MAX              10
#define RED_HUE_HIGH_MIN             170
#define RED_HUE_HIGH_MAX             180
#define RED_SATURATION_MIN           90
#define RED_BRIGHTNESS_MIN           80
#define RED_HSV_CHANNEL_MAX          255

#define RED_MARKER_MIN_LONG_SIDE     3             // 红色轮廓旋转外接框的最小长边
#define RED_MARKER_MIN_SHORT_SIDE    1             // 红色轮廓旋转外接框的最小短边
#define RED_MARKER_MIN_AREA          8             // 外接框内允许的最少红色像素数
#define RED_MARKER_MAX_AREA          10000           // 外接框内允许的最多红色像素数
#define RED_MARKER_MIN_ASPECT        1.5             // 旋转外接框最小长短边比
#define RED_MARKER_MAX_ASPECT        15            // 旋转外接框最大长短边比
#define RED_MARKER_DETECT_TOP_ROW    25            // 红板检测的最小起始行，实际起始行还会受最长白列限制
#define RED_MARKER_DETECT_BOTTOM_MARGIN 10         // 不检测画面最底部的行数
#define RED_MARKER_DETECT_LEFT_MARGIN 25           // 不检测画面左侧的列数
#define RED_MARKER_DETECT_RIGHT_MARGIN 25          // 不检测画面右侧的列数
#define RED_MARKER_CENTER_TOLERANCE  20            // 红板中心相对赛道中点允许的最大横向偏差

// 160x120 处理图中的 32x32 框，对应 320x240 原图中的 64x64 模型输入。
#define IMAGE_ROI_SIZE               32
#define RED_RECOGNITION_BOTTOM_ROW     50         // 红板底边行号大于该值时开始识别（使用 160x120 坐标）

typedef enum
{
    RED_AVOIDANCE_STATE_IDLE = 0,              // 未检测到红色物块，正常巡线
    RED_AVOIDANCE_STATE_WAIT_DIRECTION,        // 已检测红色物块，等待图片模型给出绕行方向
    RED_AVOIDANCE_STATE_LEFT,                  // 向左绕行
    RED_AVOIDANCE_STATE_RIGHT,                 // 向右绕行
    RED_AVOIDANCE_STATE_STRAIGHT,              // 保持赛道中线直行
    RED_AVOIDANCE_STATE_RETURN_TRACK           // 绕过障碍物，回归赛道并等待参考亮度恢复
} RedAvoidanceState;

typedef enum
{
    RED_AVOIDANCE_DIRECTION_UNKNOWN = 0,
    RED_AVOIDANCE_DIRECTION_LEFT,
    RED_AVOIDANCE_DIRECTION_RIGHT,
    RED_AVOIDANCE_DIRECTION_STRAIGHT
} RedAvoidanceDirection;

typedef struct
{
    uint8_t left_boundary[SEARCH_UVC_HEIGHT];   // 每一行扫出来的左边界列号
    uint8_t right_boundary[SEARCH_UVC_HEIGHT];  // 每一行扫出来的右边界列号
    uint8_t center_line[SEARCH_UVC_HEIGHT];     // 每一行根据左右边界算出的中线列号
    uint8_t left_found[SEARCH_UVC_HEIGHT];      // 该行左边界是否真的找到
    uint8_t right_found[SEARCH_UVC_HEIGHT];     // 该行右边界是否真的找到
    uint8_t valid_row_start;                    // 本帧有效巡线区的起始行
    uint8_t valid_row_end;                      // 本帧有效巡线区的结束行
    uint8_t cross_flag;                         // 是否检测到十字/大开口并完成过补线
    uint8_t reference_col;                      // 最远参考列，后续各行都从这里向左右扫
    uint8_t reference_row;                      // 参考列对应的最远参考行，也就是起扫行
    uint8_t red_found;                          // 是否检测到红色标识纸
    uint8_t red_avoidance_state;                // 红色物块绕行状态，取值见 RedAvoidanceState
    uint8_t red_avoidance_direction;            // 图片模型识别的绕行方向，取值见 RedAvoidanceDirection
    uint8_t red_x;                              // 红色标识纸外接框左上角 x
    uint8_t red_y;                              // 红色标识纸外接框左上角 y
    uint8_t red_w;                              // 红色标识纸外接框宽度
    uint8_t red_h;                              // 红色标识纸外接框高度
    uint8_t red_corner_x[4];                    // 红色标识纸水平外接框四个角点 x
    uint8_t red_corner_y[4];                    // 红色标识纸水平外接框四个角点 y
    uint8_t image_found;                        // 是否已生成模型输入框
    uint8_t image_x;                            // 模型输入框左上角 x
    uint8_t image_y;                            // 模型输入框左上角 y
    uint8_t image_w;                            // 模型输入框宽度
    uint8_t image_h;                            // 模型输入框高度
    uint8_t image_corner_x[4];                  // 模型输入框四个角点 x
    uint8_t image_corner_y[4];                  // 模型输入框四个角点 y
    int16_t line_error;                         // 有效中线 0% 到 30% 近端区平均值相对图像中心的偏差，0% 在底部，100% 在顶部
    int16_t heading_error;                        // 有效中线 30% 到 70% 远端区平均值与近端区平均值的差，0% 在底部，100% 在顶部
} VisionResult;

// 计算本帧的白赛道参考亮度，并生成动态判白阈值、判黑阈值。
void Get_Reference_Point(const uint8_t *pImage);

// 搜索“最远还在白赛道里的那一列”，记为参考列和参考行。
void Get_Reference_Col(const uint8_t *pImage);

// 从参考列开始，逐行向左右搜索边界。
void Search_Boundary(const uint8_t *pImage, VisionResult &result);

// 十字/大开口处理：若连续多行出现缺边或宽度异常增大，则尝试按前后支撑行补线。
void Cross_Check_And_Repair(VisionResult &result);

// 根据边界计算中线、有效区间、丢线标志和控制误差。
void Calculate_Center_Line(VisionResult &result);

// 单帧视觉总入口。
void Process_Vision(const uint8_t *pImage, VisionResult &result);

// 给主函数调度用的外层入口，目前只是简单包一层，后面可扩展异帧策略。
void Process_Vision_Frame(const uint8_t *pImage,
                          const cv::Mat &color_frame,
                          VisionResult &result);

// 红色物块绕行状态机。
void Red_Avoidance_State_Machine(VisionResult &result);

// 图片模型接入后，用这个接口写入左绕或右绕结果。
void Set_Red_Avoidance_Direction(VisionResult &result, RedAvoidanceDirection direction);

// 编码器到值后恢复正常中线，开始回归赛道。
void Start_Red_Avoidance_Return(VisionResult &result);

// 底部参考亮度恢复后完全退出绕行。
void Reset_Red_Avoidance(VisionResult &result);

// 把视觉参考对比度参数同步到逐飞助手参数通道，方便上位机显示默认值。
void vision_sync_reference_contrast_to_assistant(void);

// 从逐飞助手参数通道更新视觉 reference_contrast_ratio。
void vision_update_reference_contrast_from_assistant(void);

#endif
