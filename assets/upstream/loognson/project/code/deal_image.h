#ifndef _DEAL_IMAGE_H_
#define _DEAL_IMAGE_H_

#include "zf_common_headfile.hpp"

/****************************** image tracing ******************************/
#define  RANGE_COLUMN               (8)         // 寻找预测列的阈值
#define  FAR_SEARCH_ROW             (80)
#define  NEAR_SEARCH_ROW            (40)
/****************************** cross road ******************************/
#define  SEARCH_HEIGHT              (20)
#define  CROSS_RANG_ROW             (5)
#define  CROSS_RANG_COL             (10)
/****************************** zebra line ******************************/
#define  ZEBRA_START_ROW            (70)        // 斑马线的起始行
#define  ZEBRA_END_ROW              (68)        // 斑马线的结束行
#define  ZEBRA_MAX_TIMES            (9)         // 判断斑马线最大个数
#define  ZEBRA_MIN_TIMES            (4)         // 判断斑马线最小个数
/****************************** obstacle ******************************/
#define  OBSTACLE_JUDGE_DIFFER      (16)        // 判断避障的误差阈值
#define  OBSTACLE_JUDGE_ROW         (48)        // 判断避障的丢线行
#define  OBSTACLE_DISTANCE          (3800)      // 计算退出避障的距离 19cm
/****************************** ramp ******************************/
#define  RAMP_JUDGE_START_ROW       (70)                                    // 判断坡道的起始行
#define  RAMP_JUDGE_END_ROW         (40)                                    // 判断坡道的结束行
#define  RAMP_JUDGE_WIDTH           (10)                                    // (赛道宽度 - 直道宽度)的阈值
#define  RAMP_JUDGE_TIMES           ((RAMP_JUDGE_START_ROW - RAMP_JUDGE_END_ROW)/3)     // 判断坡道的次数
#define  RAMP_STANDER_WIDTH         (42)                                    // 单边边线的标准阈值
#define  RAMP_DIFFER_ROW            (85)                                    // 坡道计算误差行
#define  RAMP_DISTANCE              (8000)                                 // 计算退出坡道的距离 0.8m
/****************************** island ******************************/
#define  JUDGE_ISLAND_MAX_ROW       (40)        // 判断环岛的最大行
#define  JUDGE_ISLAND_MIN_ROW       (54)        // 判断环岛的最小行
#define  JUDGE_ISLAND_WHITE_COUNT   (15)        // 判断环岛的白色像素数
#define  JUDGE_ISLAND_BLACK_COUNT   (7)         // 判断环岛的黑色像素数
/****************************** straight ******************************/
#define  JUDGE_STRAIGHT_ROW         (70)        // 判断直道的行
#define  JUDGE_STRAIGHT_SLOPE       (0.72)      // 边界斜率

/****************************** image tracing ******************************/
extern uint8_t want_start_row;
extern uint8_t want_end_row;
/****************************** function ******************************/
struct point // 点
{
    uint8_t x;
    uint8_t y;
};
/****************************** element ******************************/
struct crossroad // 十字路口
{
    bool cross_flag;
    bool cross_left_flag;
    bool cross_right_flag;
    int16_t left_blank_height;
    int16_t right_blank_height;
};
extern struct crossroad judge_crossroad;
struct zebra_line // 斑马线
{
    uint8_t flag;
};
extern struct zebra_line judge_zebra_line;
struct roadblock // 避障
{
    bool left_roadblock_flag;
    bool right_roadblock_flag;
    uint8_t left_line;
    uint8_t right_line;
    int32_t encoder_count;
};
extern struct roadblock judge_roadblock;
struct ramp // 坡道
{
    bool flag;
    uint8_t left_width;
    uint8_t right_width;
    int32_t encoder_count;
};
extern struct ramp judge_ramp;
extern uint8_t road_differ[120];
struct island // 环岛
{
    uint8_t range_row;
    uint8_t range_column;
    bool out_island_flag;
    uint8_t left_island_type;
    bool left_type1_flag;
    float left_type2_k;
    float left_type2_b;
    float left_type4_k;
    float left_type4_b;
    uint8_t right_island_type;
    bool right_type1_flag;
    float right_type2_k;
    float right_type2_b;
    float right_type4_k;
    float right_type4_b;
};
extern struct island judge_island;
struct cooldown // 元素冷却
{
    uint8_t roadblock_cooldown;
    uint8_t ramp_cooldown;
    uint8_t island_cooldown;
};
extern struct cooldown element_cooldown;

/****************************** image tracing ******************************/
void search_edge_init(void); // 初始化搜索边界
void search_image_edge(void); // 搜索边界
void search_line_error(void); // 搜索线错误
/****************************** function ******************************/
int16_t calculate_cross_product(point p1, point p2, point p3); // 计算叉积
bool judge_on_segment(point p1, point p2, point p3); // 判断是否在线段
bool judge_intersect(point left_start_point, point left_end_point, point right_start_point, point right_end_point); // 判断是否相交
/****************************** element ******************************/
void search_cross_road(void); // 搜索十字路口
void search_zebra_line(void); // 搜索斑马线
void search_roadblock(void); // 搜索避障
void search_ramp(void); // 搜索坡道
void search_left_island(void); // 搜索左环岛
void search_right_island(void); // 搜索右环岛
void search_long_straight(void); // 搜索直道
void judge_element(void); // 判断元素
void element_reminder(void); // 元素提醒

#endif
