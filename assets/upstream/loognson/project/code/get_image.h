#ifndef _GET_IMAGE_H_
#define _GET_IMAGE_H_

#include "zf_common_headfile.hpp"

/****************************** image size ******************************/
#define ROW                        UVC_HEIGHT/2     // 图像行数
#define COL                        UVC_WIDTH/2      // 图像列数
#define DETECT_MIN_ROW             (100)            // 色块检测最小行
#define DETECT_MAX_ROW             (210)            // 色块检测最大行   
#define DETECT_MIN_COLUMN          (40)             // 色块检测最小列
#define DETECT_MAX_COLUMN          (280)            // 色块检测最大列
#define WHITE                      (255)            // 白色
#define GRAY                       (100)            // 灰色
#define BLACK                      (0)              // 黑色
/****************************** verge size ******************************/
#define LEFT_BOUNDARY_COLUMN       (2)              // 左边界列
#define RIGHT_BOUNDARY_COLUMN      (158)            // 右边界列
#define WANT_START_ROW             (110)            // 起始行
#define WANT_END_ROW               (30)             // 结束行
#define CHANGE_COLUMN              (4)              // 变化列
#define CBH_THRESHOLD              (6)              // CBH阈值
#define LONGEST_END_ROW            (ROW/2)          // 最长白列的结束行
#define START_MIDDLE_COLUMN        (COL/2)          // 中间列
#define EXTRA_ROWS                 (3)              // 边界外推行数
#define BLEND_ROWS                 (4)              // 渐变混合行数
/****************************** calculate image differ ******************************/
#define FAR_DIFFER_MAX             (60.0)           // 60.0 远端偏差最大
#define NEAR_DIFFER_MAX            (20.0)           // 20.0 近端偏差最大
#define BLDC_DIFFER_MAX            (20.0)           // 20.0 BLDC偏差最大
#define AMPLIFICATION_DIFFER       (30.0)           // 30.0 放大偏差标准值
#define MAX_DIFFER_BIAS            (50.0)           // 50.0 基础转向偏差阈值
#define IMAGE_DIFFER_MAX           (180.0)          // 180.0 图像偏差最大
#define PID_DIFFER_MAX             (500.0)          // 500.0 PID图像偏差最大
#define RAMP_MAX_DIFFER            (24.0)           // 坡道偏差最大值
#define ROADBLOCK_MAX_DIFFER       (16.0)           // 避障偏差最大值

/****************************** basic image ******************************/
extern uint16_t* rgb_image;                         // RGB图像指针
extern uint8_t* gray_image;                         // 灰度图像指针
extern uint8_t image_buffer[ROW][COL];              // 图像缓冲区
extern float adaption_threshold;                    // 自适应阈值
/****************************** TCP ******************************/
extern uint16_t rgb_image_tcp[UVC_HEIGHT][UVC_WIDTH];
extern uint8_t gray_image_tcp[ROW][COL];
extern uint8_t image_buffer_tcp[ROW][COL];

/****************************** deal basic image ******************************/
struct boundary_element // 边界元素
{
    uint8_t row;                                    // 行
    uint8_t column;                                 // 列
    uint8_t flag;                                   // 标志位
};
struct boundary // 边界
{
    struct boundary_element left[ROW];              // 边界
    struct boundary_element right[ROW];
    struct boundary_element left_inflection;        // 拐点
    struct boundary_element right_inflection;
    struct boundary_element left_first_point;       // 开始
    struct boundary_element right_first_point;
    struct boundary_element left_last_point;        // 结束
    struct boundary_element right_last_point;
    struct boundary_element left_lose_line;         // 丢线
    struct boundary_element right_lose_line;
};
extern struct boundary verge;
struct white_column // 最长白列
{
    uint8_t row;                                    // 行
    uint8_t column;                                 // 列
    int16_t IMAGE_CBH;                              // 图像CBH
    struct boundary_element down[COL];
    struct boundary_element last_down[COL];
    struct boundary_element llast_down[COL];
};
extern struct white_column longest_white_column;    // 最长白列
extern uint8_t longest_white_column1;               // 左边最长白列
extern uint8_t longest_white_row1;                  // 左边最长白行
extern uint8_t longest_white_column2;               // 右边最长白列
extern uint8_t longest_white_row2;                  // 右边最长白行
/****************************** calculate image differ ******************************/
struct middle_line // 中线
{
    uint8_t middle_end_row;                         // 中线结束行
    struct boundary_element middle[ROW];            // 中线元素
};
extern struct middle_line center;
struct image_differ // 图像偏差
{
    uint8_t far;                                    // 远端偏差行
    uint8_t near;                                   // 近端偏差行
    uint8_t BLDC;                                   // BLDC偏差行
    float differ_far;                               // 远端偏差
    float differ_near;                              // 近端偏差
    float differ_BLDC;                              // BLDC偏差
    float differ_decision;                          // 决策偏差
    float last_differ_far;                          // 上一次远端偏差
    float last_differ_near;                         // 上一次近端偏差
    float last_differ_BLDC;                         // 上一次BLDC偏差
};
extern struct image_differ angle_line;

/****************************** display image ******************************/
void display_image(void); // 显示图像
/****************************** deal basic image ******************************/
void search_left_edge(uint8_t row, uint8_t start_column, uint8_t end_column); // 搜索左边界
void search_right_edge(uint8_t row, uint8_t start_column, uint8_t end_column); // 搜索右边界
void search_left_inflection(uint8_t start_row, uint8_t end_row); // 搜索左拐点
void search_right_inflection(uint8_t start_row, uint8_t end_row); // 搜索右拐点
void search_longest_row(uint8_t start_row, uint8_t end_row, uint8_t column); // 搜索最长白行
int16_t calc_diff(int16_t x, int16_t y); // 计算差值
/****************************** calculate image differ ******************************/
void obtain_middle_line(void); // 获取中线
void smooth_middle_line(void); // 平滑中线
void deal_image_differ(void); // 处理图像偏差
void calculate_far_differ(uint8_t far_angle_line); // 计算远端偏差
void calculate_near_differ(uint8_t near_angle_line); // 计算近端偏差
void calculate_BLDC_differ(uint8_t BLDC_angle_line); // 计算BLDC偏差
/****************************** function ******************************/
int16_t add_line(int16_t x, float k, float b); // 添加边线
void linear_function_draw_line(uint8_t image_buffer[][COL], uint8_t start_row, uint8_t end_row, float k, float b, uint8_t type); // 绘制边线
uint8_t search_black_white_transition(uint8_t image_buffer[][COL], uint8_t start_row, uint8_t end_row, uint8_t target_column, uint8_t type); // 搜索黑白过渡
int16_t find_max(int16_t a, int16_t b); // 查找最大值
int16_t find_min(int16_t a, int16_t b); // 查找最小值
float limit_value(float value, float max_value, float min_value); // 限制值

#endif
