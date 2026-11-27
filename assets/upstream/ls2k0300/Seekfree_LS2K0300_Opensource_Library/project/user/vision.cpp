#include "vision.hpp"

#include <cstring>
void yaw_start_cout(uint8_t state);
float yaw_get_from_imu(void);

static uint8_t refer_white_point = 0;           // 本帧底部白赛道的平均亮度。
static uint8_t white_max_refer = 0;             // 判定“白区”时使用的上阈值。
static uint8_t white_min_refer = 0;             // 判定“黑区/明显变暗”时使用的下阈值。
static uint8_t reference_col = 0;               // 本帧最远参考列。
static uint8_t reference_point = 0;             // 本帧最远参考列对应的起扫行。
static uint8_t reference_contrast_ratio = 35;   // 灰度突变判定阈值，数值越大越保守。
static uint8_t remote_distance[SEARCH_COL_COUNT] = {0}; // 记录每个采样列最远离开白赛道的位置。
static uint8_t circle_reference_limit_enable = 0; // 入圆环后限制最长白列继续扫到外侧直道。
static int circle_reference_limit_col = -1;       // 最长白列扫描限制列。
static int8_t circle_reference_limit_side = 0;    // 限幅方向，1 跳过限制列右侧，-1 跳过限制列左侧；用于左右圆环共用同一套参考列扫描。



static constexpr uint8_t TRACK_LEFT_EDGE = 0;
static constexpr uint8_t TRACK_RIGHT_EDGE = SEARCH_UVC_WIDTH - 1;
static constexpr uint8_t REFERENCE_ROW_OFFSET = 1 * CONTRASTOFFSET; // 起扫行下移偏移量，跳过参考点最远端附近的不稳定区域。
static constexpr uint8_t NEAR_PERCENT_START = 0;                    // 近端区起始位置，占有效中线长度的百分比，0% 在底部，100% 在顶部。
static constexpr uint8_t NEAR_PERCENT_END = 40;                     // 近端区结束位置，占有效中线长度的百分比，0% 在底部，100% 在顶部。
static constexpr uint8_t FAR_PERCENT_START = 40;                    // 远端区起始位置，占有效中线长度的百分比，0% 在底部，100% 在顶部。
static constexpr uint8_t FAR_PERCENT_END = 90;                      // 远端区结束位置，占有效中线长度的百分比，0% 在底部，100% 在顶部。
static constexpr int RED_AVOIDANCE_OFFSET = 25;                     // 红色物块绕行时，控制中线向绕行方向偏移的像素数。
static constexpr int RED_RETURN_REFERENCE_TOLERANCE_PERCENT = 10;   // 回归时底部参考亮度相对锁定值的允许偏差。


static constexpr float OUT_ROUND_DEGREE = 330.0f;                   // 出圆弧角度阈值，超过此角度认为出圆环   



static void connect_point(uint8_t *boundary, int row_a, int row_b);
static void connect_ray(uint8_t *boundary, int origin_row, int slope_row, int end_row, int row_direction);
static void Detect_Red_Marker(const cv::Mat &color_frame, VisionResult &result);
static void Calculate_Center_Error(VisionResult &result);

//==============================逐飞助手参数通道定义与差比和上位机设置==============================//
#define ASSISTANT_INDEX_VISION_CONTRAST    7

void vision_sync_reference_contrast_to_assistant(void)
{
    seekfree_assistant_parameter[ASSISTANT_INDEX_VISION_CONTRAST] = reference_contrast_ratio;
}

void vision_update_reference_contrast_from_assistant(void)
{
    if(seekfree_assistant_parameter_update_flag[ASSISTANT_INDEX_VISION_CONTRAST])
    {
        reference_contrast_ratio = (uint8_t)func_limit_ab((int)seekfree_assistant_parameter[ASSISTANT_INDEX_VISION_CONTRAST], 0, 255);
        seekfree_assistant_parameter[ASSISTANT_INDEX_VISION_CONTRAST] = reference_contrast_ratio;
        seekfree_assistant_parameter_update_flag[ASSISTANT_INDEX_VISION_CONTRAST] = 0;
    }
}

//===============================================================================================//

// 把特征搜索区间裁剪到本帧实际执行过边界搜索的行。
// 行号越大越靠近画面底部；bottom_margin/top_margin 用来给相邻点或窗口留出空间。
static bool vision_get_boundary_feature_rows(const VisionResult &result,
                                             int requested_row_a,
                                             int requested_row_b,
                                             int bottom_margin,
                                             int top_margin,
                                             int &search_bottom,
                                             int &search_top)
{
    if(result.valid_row_start >= SEARCH_UVC_HEIGHT ||
       result.valid_row_end >= SEARCH_UVC_HEIGHT ||
       result.valid_row_start > result.valid_row_end)
    {
        return false;
    }

    int requested_bottom = func_limit_ab(requested_row_a, 0, SEARCH_UVC_HEIGHT - 1);
    int requested_top = func_limit_ab(requested_row_b, 0, SEARCH_UVC_HEIGHT - 1);
    if(requested_bottom < requested_top)
    {
        int temp = requested_bottom;
        requested_bottom = requested_top;
        requested_top = temp;
    }

    const int common_bottom = (requested_bottom < result.valid_row_end) ?
                              requested_bottom : result.valid_row_end;
    const int common_top = (requested_top > result.valid_row_start) ?
                           requested_top : result.valid_row_start;

    if(common_bottom - common_top < bottom_margin + top_margin)
    {
        return false;
    }

    search_bottom = common_bottom - bottom_margin;
    search_top = common_top + top_margin;
    return search_bottom >= search_top;
}

// 通用跳变点搜索：
// jump_dir = 1 代表向右大跳，jump_dir = -1 代表向左大跳。
// 跳变方向以从图像下方向上搜索时的边界列变化为准。
int find_jump_point(const VisionResult &result,
                    const uint8_t *boundary,
                    int row_start,
                    int row_end,
                    int jump_distance,
                    int jump_dir)
{
    int search_bottom = 0;
    int search_top = 0;

    if(nullptr == boundary ||
       !vision_get_boundary_feature_rows(result,
                                         row_start,
                                         row_end,
                                         CONTRASTOFFSET,
                                         0,
                                         search_bottom,
                                         search_top))
    {
        return -1;
    }

    for(int row = search_bottom; row >= search_top; row -= CONTRASTOFFSET)
    {
        int lower_row = row + CONTRASTOFFSET;
        int delta = ((int)boundary[row] - (int)boundary[lower_row]) * jump_dir;
        if(delta >= jump_distance)
        {
            return row;
        }
    }

    return -1;
}

// 通用边界跳变点计数：
// 1. boundary 传左右边界数组；
// 2. 只根据相邻采样行的边界列变化判断跳变；
// 3. jump_dir = 1 统计向右大跳，jump_dir = -1 统计向左大跳。
// 跳变方向以从图像下方向上搜索时的边界列变化为准。
uint8 find_jump_point_count(const VisionResult &result,
                            const uint8_t *boundary,
                            int row_start,
                            int row_end,
                            int jump_distance,
                            int jump_dir)
{
    int jump_count = 0;
    int search_bottom = 0;
    int search_top = 0;

    if(nullptr == boundary ||
       !vision_get_boundary_feature_rows(result,
                                         row_start,
                                         row_end,
                                         CONTRASTOFFSET,
                                         0,
                                         search_bottom,
                                         search_top))
    {
        return 0;
    }

    for(int row = search_bottom; row >= search_top; row -= CONTRASTOFFSET)
    {
        int lower_row = row + CONTRASTOFFSET;
        int delta = ((int)boundary[row] - (int)boundary[lower_row]) * jump_dir;
        if(delta >= jump_distance)
        {
            jump_count++;
        }
    }

    return (uint8)func_limit_ab(jump_count, 0, 255);
}

// 通用连续丢线长度统计：
// found 传 left_found/right_found，返回起始行到终止行连续未找到边界的行数。
int vision_count_bottom_missing(const uint8_t *found,
                                int row_start,
                                int row_end)
{
    int missing_count = 0;
    int row = 0;

    if(nullptr == found)
    {
        return 0;
    }

    row_start = func_limit_ab(row_start, 0, SEARCH_UVC_HEIGHT - 1);
    row_end = func_limit_ab(row_end, 0, SEARCH_UVC_HEIGHT - 1);

    if(row_start < row_end)
    {
        int temp = row_start;
        row_start = row_end;
        row_end = temp;
    }

    for(row = row_start; row >= row_end; row -= CONTRASTOFFSET)
    {
        if(0 != found[row])
        {
            break;
        }

        missing_count++;
    }

    return missing_count;
}

// 通用非连续丢线行数统计：
// found 传 left_found/right_found，返回给定区间内未找到边界的总行数。
int vision_count_missing_rows(const uint8_t *found,
                              int row_start,
                              int row_end)
{
    int missing_count = 0;

    if(nullptr == found)
    {
        return 0;
    }

    row_start = func_limit_ab(row_start, 0, SEARCH_UVC_HEIGHT - 1);
    row_end = func_limit_ab(row_end, 0, SEARCH_UVC_HEIGHT - 1);

    if(row_start < row_end)
    {
        int temp = row_start;
        row_start = row_end;
        row_end = temp;
    }

    for(int row = row_start; row >= row_end; row -= CONTRASTOFFSET)
    {
        if(0 == found[row])
        {
            missing_count++;
        }
    }

    return missing_count;
}



// 通用边界回折点搜索：
// first_dir = -1 时找先向左再向右的最左点；
// first_dir = 1 时找先向右再向左的最右点。
// 方向以从图像下方向上搜索时的边界列变化为准。
int find_boundary_turn_point(const VisionResult &result,
                             const uint8_t *boundary,
                             int row_start,
                             int row_end,
                             int first_dir,
                             int rebound_distance)
{
    const int window = 5;
    int search_bottom = 0;
    int search_top = 0;

    if(nullptr == boundary ||
       !vision_get_boundary_feature_rows(result,
                                         row_start,
                                         row_end,
                                         window,
                                         window,
                                         search_bottom,
                                         search_top))
    {
        return -1;
    }

    for(int row = search_bottom; row >= search_top; row -= CONTRASTOFFSET)
    {
        uint8_t same_count = 0;
        uint8_t turn_count = 0;
        uint8_t rebound_ok = 0;

        for(int offset = 1; offset <= window; offset++)
        {
            if(boundary[row] == boundary[row + offset] &&
               boundary[row] == boundary[row - offset])
            {
                same_count++;
            }

            if(first_dir < 0)
            {
                if(boundary[row] <= boundary[row + offset] &&
                   boundary[row] <= boundary[row - offset])
                {
                    turn_count++;
                }

                if(boundary[row + offset] >= boundary[row] + rebound_distance ||
                   boundary[row - offset] >= boundary[row] + rebound_distance)
                {
                    rebound_ok = 1;
                }
            }
            else if(first_dir > 0)
            {
                if(boundary[row] >= boundary[row + offset] &&
                   boundary[row] >= boundary[row - offset])
                {
                    turn_count++;
                }

                if(boundary[row + offset] <= boundary[row] - rebound_distance ||
                   boundary[row - offset] <= boundary[row] - rebound_distance)
                {
                    rebound_ok = 1;
                }
            }
        }

        if(same_count >= window)
        {
            continue;
        }

        if(turn_count >= window && rebound_ok)
        {
            return row;
        }
    }

    return -1;
}

// 左圆环入口补线状态机：
// 1. 默认巡线时先在左边界找“入口跳变点”和“内圆回折顶点”；
// 2. 确认右边界稳定、左边界存在缺线后，把入口跳变点连到内圆顶点；
// 3. 车辆更靠近圆环、入口跳变点消失后，改用左下角连接内圆顶点；
// 4. 左侧边界重新稳定时，认为车头准备入环，开始用右边界辅助补线;
void Round_Check_And_Repair(VisionResult &result)
{
    static uint8 round_state=0; // 圆环状态机当前状态，0 表示未识别到圆环。
    static uint8 round_direction=0; // 当前识别出的圆环方向，0-未识别 1-左圆环 2-右圆环。
    static int jump_point_row=0; // 圆环入口原始跳变点行号。
    static int round_top_row=0; // 圆环内侧边界回折点行号，用来定位内圆上沿。
    static int top_row=0; // 巡线顶端行数。
    static int exit_search_end=15;

  // printf("state:%d\n\r", round_state);
//     printf("round_dir:%d\n\r", round_direction);
  //  printf("degree:%f\n\r", yaw_get_from_imu());

    if(0 == round_state)
    {
        // 状态 0 只负责识别入口方向。
        // 每次回到状态 0，都先清掉上一轮圆环留下的最长白列限幅和方向信息。
        circle_reference_limit_enable = 0;
        circle_reference_limit_col = -1;
        circle_reference_limit_side = 0;
        round_direction = 0;
        exit_search_end = 15;

        // 当前帧发现红色，或已经进入减速、绕行、回归流程时，不再识别新的圆环入口。
        // 该门禁只位于状态 0；已经进入圆环状态 1~6 后不受红色状态影响。
        if(result.red_found || RED_AVOIDANCE_STATE_IDLE != result.red_avoidance_state)
        {
            return;
        }

        // 先按左圆环入口判定：左边界向左跳，随后在左边界上找到内圆回折点。
        jump_point_row = find_jump_point(result,result.left_boundary,119,25,10,-1);
        round_top_row = -1;
        if(jump_point_row >= 0)
        {
            round_top_row = find_boundary_turn_point(result,result.left_boundary,jump_point_row,0,1,2);
        }

        if(jump_point_row >= 0 &&
           find_jump_point_count(result,result.right_boundary,119,10,10,1) == 0 &&
           find_jump_point_count(result,result.right_boundary,119,10,10,-1) == 0 &&
           round_top_row >= 0 &&
           jump_point_row - round_top_row > 20 &&
           vision_count_missing_rows(result.left_found,jump_point_row,round_top_row) >= 3 &&
           vision_count_missing_rows(result.right_found,jump_point_row,round_top_row) <= 3)
        {
            round_state = 1;
            round_direction = 1;
            connect_point(result.left_boundary,round_top_row,jump_point_row + 1);
//             printf("左环状态一补线 jump:(%d,%d) top:(%d,%d)\n\r",
//                    jump_point_row + 1, result.left_boundary[jump_point_row + 1],
//                    round_top_row, result.left_boundary[round_top_row]);
            return;
        }

        // 左圆环条件不满足时，再按右圆环入口判定：
        // 右边界向右跳，随后在右边界上找到内圆回折点。
        jump_point_row = find_jump_point(result,result.right_boundary,119,25,10,1);
        round_top_row = -1;
        if(jump_point_row >= 0)
        {
            round_top_row = find_boundary_turn_point(result,result.right_boundary,jump_point_row,0,-1,2);
        }

        if(jump_point_row >= 0 &&
           find_jump_point_count(result,result.left_boundary,119,10,10,1) == 0 &&
           find_jump_point_count(result,result.left_boundary,119,10,10,-1) == 0 &&
           round_top_row >= 0 &&
           jump_point_row - round_top_row > 20 &&
           vision_count_missing_rows(result.right_found,jump_point_row,round_top_row) >= 3 &&
           vision_count_missing_rows(result.left_found,jump_point_row,round_top_row) <= 3)
        {
            round_state = 1;
            round_direction = 2;
            connect_point(result.right_boundary,round_top_row,jump_point_row + 1);
//             printf("右环状态一补线 jump:(%d,%d) top:(%d,%d)\n\r",
//                    jump_point_row + 1, result.right_boundary[jump_point_row + 1],
//                    round_top_row, result.right_boundary[round_top_row]);
        }
        return;
    }
    else if(1 == round_state)
    {
        // 状态 1 开始按 round_direction 分左右处理。
        // 左右圆环共用一套状态号，只是参与判定和补线的边界互换。
        if(1 == round_direction)
        {
            jump_point_row = find_jump_point(result,result.left_boundary,110,25,10,-1);
            round_top_row = -1;
            if(jump_point_row >= 0)
            {
                round_top_row = find_boundary_turn_point(result,result.left_boundary,jump_point_row - 4,0,1,2);
            }

            if(jump_point_row >= 0 && round_top_row >= 0 && jump_point_row - round_top_row > 20 &&
               vision_count_missing_rows(result.left_found,jump_point_row,round_top_row) >= 3)
            {
                connect_point(result.left_boundary,round_top_row,jump_point_row + 1);
//                 printf("左环状态一补线 jump:(%d,%d) top:(%d,%d)\n\r",
//                        jump_point_row + 1, result.left_boundary[jump_point_row + 1],
//                        round_top_row, result.left_boundary[round_top_row]);
            }

            int near_top_row = find_boundary_turn_point(result,result.left_boundary,50,10,1,2);
            if(vision_count_missing_rows(result.left_found,119,60) >= 50 &&
                vision_count_missing_rows(result.left_found,119,60) <=80 &&
               near_top_row >= 0 &&
               find_jump_point(result,result.left_boundary,119,25,10,-1) < 0)
            {
                round_top_row = near_top_row;
                round_state = 2;
                connect_point(result.left_boundary,round_top_row,SEARCH_UVC_HEIGHT - 1);
            }
        }
        else if(2 == round_direction)
        {
            jump_point_row = find_jump_point(result,result.right_boundary,110,25,10,1);
            round_top_row = -1;
            if(jump_point_row >= 0)
            {
                round_top_row = find_boundary_turn_point(result,result.right_boundary,jump_point_row - 4,0,-1,2);
            }

            if(jump_point_row >= 0 && round_top_row >= 0 && jump_point_row - round_top_row > 20 &&
               vision_count_missing_rows(result.right_found,jump_point_row,round_top_row) >= 3)
            {
                connect_point(result.right_boundary,round_top_row,jump_point_row + 1);
//                 printf("右环状态一补线 jump:(%d,%d) top:(%d,%d)\n\r",
//                        jump_point_row + 1, result.right_boundary[jump_point_row + 1],
//                        round_top_row, result.right_boundary[round_top_row]);
            }

            int near_top_row = find_boundary_turn_point(result,result.right_boundary,50,10,-1,2);
            if(vision_count_missing_rows(result.right_found,119,60) >= 40 &&
               vision_count_missing_rows(result.right_found,119,60) <=80 &&
               near_top_row >= 0 &&
               find_jump_point(result,result.right_boundary,119,25,10,1) < 0)
            {
                round_top_row = near_top_row;
                round_state = 2;
                connect_point(result.right_boundary,round_top_row,SEARCH_UVC_HEIGHT - 1);
            }
        }
        return;
    }
    else if(2 == round_state)
    {
        if(1 == round_direction)
        {
            round_top_row = find_boundary_turn_point(result,result.left_boundary,65,20,1,2);
//             printf("left_round_top_row:%d\n\r", round_top_row);
            if(round_top_row >= 0)
            {
                connect_point(result.left_boundary,round_top_row,SEARCH_UVC_HEIGHT - 1);
//                 printf("左环状态二补线 top:(%d,%d) bottom:(%d,%d)\n\r",
//                        round_top_row, result.left_boundary[round_top_row],
//                        SEARCH_UVC_HEIGHT - 1, result.left_boundary[SEARCH_UVC_HEIGHT - 1]);
            }

            if(vision_count_missing_rows(result.left_found,119,80) < 1)
            {
                round_state = 3;
                jump_point_row = find_jump_point(result,result.left_boundary,80,10,30,1);
//                 printf("左环跳变点:%d\n\r", jump_point_row);
                if(jump_point_row >= 0)
                {
                    result.right_boundary[jump_point_row] = result.left_boundary[jump_point_row];
                    circle_reference_limit_enable = 1;
                    circle_reference_limit_side = 1;
                    circle_reference_limit_col = func_limit_ab((int)result.right_boundary[jump_point_row] - 5, 0, SEARCH_UVC_WIDTH - 1);
                    connect_point(result.right_boundary,jump_point_row,SEARCH_UVC_HEIGHT - 1);
//                     printf("左环状态三补线 turn:(%d,%d) bottom:(%d,%d)\n\r",
//                            jump_point_row, result.right_boundary[jump_point_row],
//                            SEARCH_UVC_HEIGHT - 1, result.right_boundary[SEARCH_UVC_HEIGHT - 1]);
                }
            }
        }
        else if(2 == round_direction)
        {
            round_top_row = find_boundary_turn_point(result,result.right_boundary,65,20,-1,2);
//             printf("right_round_top_row:%d\n\r", round_top_row);
            if(round_top_row >= 0)
            {
                connect_point(result.right_boundary,round_top_row,SEARCH_UVC_HEIGHT - 1);
//                 printf("右环状态二补线 top:(%d,%d) bottom:(%d,%d)\n\r",
//                        round_top_row, result.right_boundary[round_top_row],
//                        SEARCH_UVC_HEIGHT - 1, result.right_boundary[SEARCH_UVC_HEIGHT - 1]);
            }

            if(vision_count_missing_rows(result.right_found,119,80) < 1)
            {
                round_state = 3;
                jump_point_row = find_jump_point(result,result.right_boundary,80,10,30,-1);
//                 printf("右环跳变点:%d\n\r", jump_point_row);
                if(jump_point_row >= 0)
                {
                    result.left_boundary[jump_point_row] = result.right_boundary[jump_point_row];
                    circle_reference_limit_enable = 1;
                    circle_reference_limit_side = -1;
                    circle_reference_limit_col = func_limit_ab((int)result.left_boundary[jump_point_row] + 5, 0, SEARCH_UVC_WIDTH - 1);
                    connect_point(result.left_boundary,jump_point_row,SEARCH_UVC_HEIGHT - 1);
//                     printf("右环状态三补线 turn:(%d,%d) bottom:(%d,%d)\n\r",
//                            jump_point_row, result.left_boundary[jump_point_row],
//                            SEARCH_UVC_HEIGHT - 1, result.left_boundary[SEARCH_UVC_HEIGHT - 1]);
                }
            }
        }
        return;
    }
    else if(3 == round_state)
    {
        yaw_start_cout(1);
        if(1 == round_direction)
        {
            jump_point_row = find_jump_point(result,result.right_boundary,119,10,20,-1);
            int jump_point_row2 = find_jump_point(result,result.left_boundary,80,10,30,1);
//             printf("left_round_state3_jump:%d\n\r", jump_point_row2);
            if(jump_point_row >= 0)
            {
//                 printf("left_round_state3_jump:%d\n\r", jump_point_row);
                connect_point(result.right_boundary,jump_point_row,SEARCH_UVC_HEIGHT - 1);
//                 printf("左环状态三补线 turn:(%d,%d) bottom:(%d,%d)\n\r",
//                        jump_point_row, result.right_boundary[jump_point_row],
//                        SEARCH_UVC_HEIGHT - 1, result.right_boundary[SEARCH_UVC_HEIGHT - 1]);
            }
            else if(jump_point_row2 >= 0)
            {
                result.right_boundary[jump_point_row2] = result.left_boundary[jump_point_row2];
                connect_point(result.right_boundary,jump_point_row2,SEARCH_UVC_HEIGHT - 1);
            }
            else if(yaw_get_from_imu()>=60)
            {
                circle_reference_limit_enable = 0;
                circle_reference_limit_col = -1;
                circle_reference_limit_side = 0;
                round_state = 4;
            }
        }
        else if(2 == round_direction)
        {
            jump_point_row = find_jump_point(result,result.left_boundary,119,10,20,1);
            int jump_point_row2 = find_jump_point(result,result.right_boundary,80,10,30,-1);
//             printf("right_round_state3_jump:%d\n\r", jump_point_row2);
            if(jump_point_row >= 0)
            {
//                 printf("right_round_state3_jump:%d\n\r", jump_point_row);
                connect_point(result.left_boundary,jump_point_row,SEARCH_UVC_HEIGHT - 1);
//                 printf("右环状态三补线 turn:(%d,%d) bottom:(%d,%d)\n\r",
//                        jump_point_row, result.left_boundary[jump_point_row],
//                        SEARCH_UVC_HEIGHT - 1, result.left_boundary[SEARCH_UVC_HEIGHT - 1]);
            }
            else if(jump_point_row2 >= 0)
            {
                result.left_boundary[jump_point_row2] = result.right_boundary[jump_point_row2];
                connect_point(result.left_boundary,jump_point_row2,SEARCH_UVC_HEIGHT - 1);
            }
            else if(yaw_get_from_imu()<=-60)
            {
                circle_reference_limit_enable = 0;
                circle_reference_limit_col = -1;
                circle_reference_limit_side = 0;
                round_state = 4;
            }
        }
        return;
    }
    else if(4 == round_state)
    {
        top_row = result.valid_row_start;

        if(1 == round_direction)
        {
            int turn_row = find_boundary_turn_point(result,result.right_boundary,119,45,-1,5);
//             printf("左环正常巡线\n\r");
            if(turn_row >= 0 && yaw_get_from_imu() >= 260.0f)
            {
                round_state = 5;
                connect_ray(result.right_boundary,
                            turn_row,
                            func_limit_ab(turn_row + 10, 0, SEARCH_UVC_HEIGHT - 1),
                            top_row,
                            -1);
//                 printf("左环状态四补线 top:(%d,%d) bottom:(%d,%d)\n\r",
//                        top_row, result.right_boundary[top_row],
//                        turn_row, result.right_boundary[turn_row]);
            }
        }
        else if(2 == round_direction)
        {
            int turn_row = find_boundary_turn_point(result,result.left_boundary,119,45,1,5);
//             printf("右环正常巡线\n\r");
            if(turn_row >= 0 && yaw_get_from_imu() <= -250.0f)
            {
                round_state = 5;
                connect_ray(result.left_boundary,
                            turn_row,
                            func_limit_ab(turn_row + 10, 0, SEARCH_UVC_HEIGHT - 1),
                            top_row,
                            -1);
//                 printf("右环状态四补线 top:(%d,%d) bottom:(%d,%d)\n\r",
//                        top_row, result.left_boundary[top_row],
//                        turn_row, result.left_boundary[turn_row]);
            }
        }
        return;
    }
    else if(5 == round_state)
    {
        top_row = result.valid_row_start;

        if(1 == round_direction)
        {
            round_top_row = find_boundary_turn_point(result,result.right_boundary,119,40,-1,5);
            if(round_top_row >= 0)
            {
                connect_ray(result.right_boundary,
                            round_top_row,
                            func_limit_ab(round_top_row + 10, 0, SEARCH_UVC_HEIGHT - 1),
                            top_row,
                            -1);
//                 printf("左环状态五补线 top:(%d,%d) bottom:(%d,%d)\n\r",
//                        top_row, result.right_boundary[top_row],
//                        round_top_row, result.right_boundary[round_top_row]);
            }

            if(yaw_get_from_imu() >= OUT_ROUND_DEGREE)
            {
                round_state = 6;
                jump_point_row = find_jump_point(result,result.left_boundary,119,15,5,1);
                if(jump_point_row >= 0)
                {
                    connect_point(result.left_boundary,jump_point_row,SEARCH_UVC_HEIGHT - 1);
                }
                yaw_start_cout(0);
            }
            else
            {
                connect_point(result.right_boundary,top_row,119);
            }
        }
        else if(2 == round_direction)
        {
            round_top_row = find_boundary_turn_point(result,result.left_boundary,119,40,1,5);
           
            if(round_top_row >= 0)
            {
                connect_ray(result.left_boundary,
                            round_top_row,
                            func_limit_ab(round_top_row + 10, 0, SEARCH_UVC_HEIGHT - 1),
                            top_row,
                            -1);
                
//                 printf("右环状态五补线 top:(%d,%d) bottom:(%d,%d)\n\r",
//                        top_row, result.left_boundary[top_row],
//                        round_top_row, result.left_boundary[round_top_row]);
            }

            if(yaw_get_from_imu() <= -OUT_ROUND_DEGREE)
            {
                round_state = 6;
                jump_point_row = find_jump_point(result,result.right_boundary,119,15,5,-1);
                if(jump_point_row >= 0)
                {
                    connect_point(result.right_boundary,jump_point_row,SEARCH_UVC_HEIGHT - 1);
                }
                yaw_start_cout(0);
            }
            else
            {
                connect_point(result.left_boundary,top_row,119);
            }
        }
        return;
    }
    else if(6 == round_state)
    {
//         printf("状态6准备出环\r\n");
        if(1 == round_direction)
        {
            jump_point_row = find_jump_point(result,result.left_boundary,119,exit_search_end,5,1);
            if(jump_point_row > 0)
            {
                connect_point(result.left_boundary,jump_point_row,SEARCH_UVC_HEIGHT - 1);
                exit_search_end = func_limit_ab(jump_point_row - 10, 15, 119);
            }
            else
            {
                round_state = 0;
                round_direction = 0;
                exit_search_end = 15;
//                 printf("左环出环完成\r\n");
            }
        }
        else if(2 == round_direction)
        {
           jump_point_row = find_jump_point(result,result.right_boundary,119,exit_search_end,5,-1);
            if(jump_point_row > 0)
            {
                connect_point(result.right_boundary,jump_point_row,SEARCH_UVC_HEIGHT - 1);
                exit_search_end = func_limit_ab(jump_point_row - 10, 15, 119);
            }
            else
            {
                round_state = 0;
                round_direction = 0;
                exit_search_end = 15;
//                 printf("右环出环完成\r\n");
            }
        }
    }
}


// 在数组给定区间内找极值下标。
// model = 0 时找最小值下标，model != 0 时找最大值下标。
uint8_t Find_Extreme_value(uint8_t *array, uint8_t lengthStart, uint8_t lengthEnd, uint8_t model)
{
    if(nullptr == array)
    {
        return 0;
    }

    if(lengthStart >= lengthEnd)
    {
        return lengthStart;
    }

    uint8_t extreme_index = lengthStart;

    for(uint16_t i = (uint16_t)lengthStart + 1; i < lengthEnd; i++)
    {
        if(0 == model)
        {
            if(array[i] < array[extreme_index])
            {
                extreme_index = (uint8_t)i;
            }
        }
        else if(array[i] > array[extreme_index])
        {
            extreme_index = (uint8_t)i;
        }
    }

    return extreme_index;
}

// 任何算出来的列号都限制在图像范围内，防止越界访问。
uint8_t vision_limit_col(int col)
{
    
    return (uint8_t)func_limit_ab(col, 0, SEARCH_UVC_WIDTH - 1);
}

//中线平均值计算，传入有效区间的起止行号，返回平均中线列号。
int vision_average_center_line(const VisionResult &result, uint8_t row_start, uint8_t row_end)
{
    uint8_t start = row_start;
    uint8_t end = row_end;
    int sum = 0;
    uint16_t count = 0;

    if(start >= SEARCH_UVC_HEIGHT)
    {
        start = SEARCH_UVC_HEIGHT - 1;
    }
    if(end >= SEARCH_UVC_HEIGHT)
    {
        end = SEARCH_UVC_HEIGHT - 1;
    }
    if(start > end)
    {
        uint8_t temp = start;
        start = end;
        end = temp;
    }

    if(start < result.valid_row_start)
    {
        start = result.valid_row_start;
    }
    if(end > result.valid_row_end)
    {
        end = result.valid_row_end;
    }

    if(start > end)
    {
        return (int)result.center_line[result.valid_row_end];
    }

    for(uint16_t row = start; row <= end; row += CONTRASTOFFSET)
    {
        sum += (int)result.center_line[row];
        count++;
    }

    if(0 == count)
    {
        return (int)result.center_line[result.valid_row_end];
    }

    return sum / (int)count;
}

//中线百分比划分函数
uint8_t vision_row_from_centerline_percent(const VisionResult &result, uint8_t percent_from_bottom)
{
    int valid_start = (int)result.valid_row_start;
    int valid_end = (int)result.valid_row_end;
    int valid_span = valid_end - valid_start;
    int offset_from_bottom = valid_span * (int)percent_from_bottom / 100;
    int row = valid_end - offset_from_bottom;

    return (uint8_t)func_limit_ab(row, valid_start, valid_end);
}

//是否为绝对白点
uint8_t vision_is_white(uint8_t gray)
{
    return (uint8_t)(gray > white_max_refer);
}

//是否为绝对黑点
uint8_t vision_is_black(uint8_t gray)
{
    return (uint8_t)(gray < white_min_refer);
}

//差比和判断
uint16_t vision_contrast_ratio(uint8_t current, uint8_t compare)
{
    // 用归一化后的灰度差来衡量“是否发生明显突变”。
    // 这样比直接比较绝对差值更不容易受整体亮度变化影响。
    uint16_t sum = (uint16_t)current + (uint16_t)compare;
    int diff = (int)current - (int)compare;

    if(diff < 0)
    {
        diff = -diff;
    }

    if(0 == sum)
    {
        return 0;
    }

    return (uint16_t)(diff * 200 / (int)sum);
}

//当视觉丢帧或输入图像异常时，左右边界都为屏幕边界
void vision_reset_result_when_lost(VisionResult &result)
{
    // 丢帧或空图像时，直接给出一份安全默认结果，避免外部读到脏数据。
    std::memset(&result, 0, sizeof(result));

    for(uint16_t row = 0; row < SEARCH_UVC_HEIGHT; row++)
    {
        result.left_boundary[row] = TRACK_LEFT_EDGE;
        result.right_boundary[row] = TRACK_RIGHT_EDGE;
        result.center_line[row] = SEARCH_UVC_WIDTH / 2;
    }
}

//在寻找边界前调用，清空前一帧的设置
void vision_prepare_boundary_result(VisionResult &result)
{
    // 每次正式扫边前，只重置和边界相关的数据，不整帧暴力清空所有结果。
    result.valid_row_start = 0;
    result.valid_row_end = SEARCH_UVC_HEIGHT - 1;
    result.cross_flag = 0;
    result.reference_col = 0;
    result.reference_row = 0;

    for(uint16_t row = 0; row < SEARCH_UVC_HEIGHT; row++)
    {
        result.left_boundary[row] = TRACK_LEFT_EDGE;
        result.right_boundary[row] = TRACK_RIGHT_EDGE;
        result.center_line[row] = SEARCH_UVC_WIDTH / 2;
        result.left_found[row] = 0;
        result.right_found[row] = 0;
    }
}

//左右搜寻边界
static uint8_t vision_find_boundary_on_side(const uint8_t *pImage,
                                            uint8_t row,
                                            int start_col,
                                            int direction,
                                            uint8_t &boundary_col)
{
    // 这个函数只负责一件事：
    // 从某一行的参考列出发，朝左或朝右找到“离开白赛道”的边界位置。
    int col = start_col;

    while(col >= 0 && col < SEARCH_UVC_WIDTH)
    {
        uint8_t current_gray = *(pImage + row * SEARCH_UVC_WIDTH + col);

        // 起扫点默认落在白赛道内部，因此先连续跳过白区，直到遇到变暗的位置。
        if(vision_is_white(current_gray))
        {
            col += direction;
            continue;
        }

        if(vision_is_black(current_gray))
        {
            boundary_col = (uint8_t)col;
            return 1;
        }

        // 不是纯黑也可能已经到了边界，和白区内侧点做一次对比度判定。
        int compare_col = col - direction * CONTRASTOFFSET;
        compare_col = func_limit_ab(compare_col, 0, SEARCH_UVC_WIDTH - 1);

        uint8_t compare_gray = *(pImage + row * SEARCH_UVC_WIDTH + compare_col);
        uint16_t ratio = vision_contrast_ratio(current_gray, compare_gray);

        if(current_gray <= compare_gray && ratio > reference_contrast_ratio)
        {
            boundary_col = (uint8_t)col;
            return 1;
        }

        col += direction;
    }

    boundary_col = (direction < 0) ? TRACK_LEFT_EDGE : TRACK_RIGHT_EDGE;
    return 0;
}

//补线函数
static void connect_point(uint8_t *boundary, int row_a, int row_b)
{
    // 把同一条边界上的两个点用直线补齐。
    // 参数说明：
    // boundary : 要补线的边界数组
    // row_a    : 第一个点所在行号
    // row_b    : 第二个点所在行号
    int row = 0;
    int start_row = row_a;
    int end_row = row_b;
    int start_col = boundary[row_a];
    int end_col = boundary[row_b];

    if(start_row > end_row)
    {
        int temp_row = start_row;
        int temp_col = start_col;

        start_row = end_row;
        end_row = temp_row;
        start_col = end_col;
        end_col = temp_col;
    }

    for(row = start_row; row <= end_row; row += CONTRASTOFFSET)
    {
        int numerator = row - start_row;
        int denominator = end_row - start_row;
        int col = start_col;

        if(denominator > 0)
        {
            col = start_col + (end_col - start_col) * numerator / denominator;
        }

        boundary[row] = vision_limit_col(col);
    }
}

//射线补线函数
static void connect_ray(uint8_t *boundary, int origin_row, int slope_row, int end_row, int row_direction)
{
    // 以 origin_row 为射线起点，用 origin_row 与 slope_row 两点确定斜率，
    // 再按 row_direction 延伸到 end_row。row_direction < 0 向图像上方补，> 0 向下方补。
    if(nullptr == boundary)
    {
        return;
    }

    origin_row = func_limit_ab(origin_row, 0, SEARCH_UVC_HEIGHT - 1);
    slope_row = func_limit_ab(slope_row, 0, SEARCH_UVC_HEIGHT - 1);
    end_row = func_limit_ab(end_row, 0, SEARCH_UVC_HEIGHT - 1);

    if(origin_row == slope_row)
    {
        return;
    }

    if(0 == row_direction)
    {
        row_direction = (end_row >= origin_row) ? 1 : -1;
    }
    else
    {
        row_direction = (row_direction > 0) ? 1 : -1;
    }

    if((row_direction > 0 && end_row < origin_row) ||
       (row_direction < 0 && end_row > origin_row))
    {
        return;
    }

    int origin_col = boundary[origin_row];
    int slope_col = boundary[slope_row];
    int row_delta = origin_row - slope_row;
    int col_delta = origin_col - slope_col;

    for(int row = origin_row;
        (row_direction > 0) ? (row <= end_row) : (row >= end_row);
        row += row_direction * CONTRASTOFFSET)
    {
        int col = origin_col + col_delta * (row - origin_row) / row_delta;
        boundary[row] = vision_limit_col(col);
    }
}


static uint8_t vision_row_is_cross_abnormal(const VisionResult &result, int row, int min_cross_width)
{
    if(row < result.valid_row_start || row > result.valid_row_end)
    {
        return 1;
    }

    if(!result.left_found[row] || !result.right_found[row])
    {
        return 1;
    }

    int width = (int)result.right_boundary[row] - (int)result.left_boundary[row];

    // 连续多行只要出现“缺边”或“宽度异常增大”，就认为进入十字/大开口区。
    if(width >= min_cross_width)
    {
        return 1;
    }

    if(result.left_boundary[row] <= TRACK_LEFT_EDGE || result.right_boundary[row] >= TRACK_RIGHT_EDGE)
    {
        return 1;
    }

    return 0;
}


static uint8_t Calculate_Bottom_Reference_Point(const uint8_t *pImage)
{
    // 只取图像最底部若干行、并且只取中间一段宽度做平均。
    // 这样可以尽量避开左右两边赛道外的暗区，让白赛道参考亮度更稳定。
    uint16_t reference_width = REFERENCE_CENTER_WIDTH;
    uint16_t start_col = 0;
    uint16_t end_col = 0;
    uint32_t temp_grey_count = 0;

    if(reference_width > SEARCH_UVC_WIDTH)
    {
        reference_width = SEARCH_UVC_WIDTH;
    }

    start_col = (SEARCH_UVC_WIDTH - reference_width) / 2;
    end_col = start_col + reference_width;

    for(uint16_t row = SEARCH_UVC_HEIGHT - REFERENCE_ROW; row < SEARCH_UVC_HEIGHT; row++)
    {
        for(uint16_t col = start_col; col < end_col; col++)
        {
            temp_grey_count += *(pImage + row * SEARCH_UVC_WIDTH + col);
        }
    }

    return (uint8_t)(temp_grey_count / (REFERENCE_ROW * reference_width));
}

//取参考白点作为边界判断点灰度值
void Get_Reference_Point(const uint8_t *pImage)
{
    refer_white_point = Calculate_Bottom_Reference_Point(pImage);
    white_max_refer = func_limit_ab((uint32_t)(refer_white_point * WHITEMAXMUL) / 10, BLACKPOINT, 255);
    white_min_refer = func_limit_ab((uint32_t)(refer_white_point * WHITEMINMUL) / 10, BLACKPOINT, 255);
}

static bool Red_Return_Reference_Matched(const uint8_t *pImage)
{
    uint8_t current_reference = Calculate_Bottom_Reference_Point(pImage);
    uint16_t reference_min = refer_white_point * (100 - RED_RETURN_REFERENCE_TOLERANCE_PERCENT) / 100;
    uint16_t reference_max = refer_white_point * (100 + RED_RETURN_REFERENCE_TOLERANCE_PERCENT) / 100;

    return current_reference >= reference_min && current_reference <= reference_max;
}

//搜寻最长白列
void Get_Reference_Col(const uint8_t *pImage)
{
    const uint8_t scan_col_count = (uint8_t)(sizeof(remote_distance) / sizeof(remote_distance[0]));

    for(uint8_t col_index = 0; col_index < scan_col_count; col_index++)
    {
        remote_distance[col_index] = SEARCH_UVC_HEIGHT - 1;
    }

    for(uint8_t col_index = 0; col_index < scan_col_count; col_index++)
    {
        int col = col_index * CONTRASTOFFSET;
        col = func_limit_ab(col, 0, SEARCH_UVC_WIDTH - 1);

        if(circle_reference_limit_enable)
        {
            // 左圆环会屏蔽限制列右侧，右圆环会屏蔽限制列左侧。
            // 这样最长白列仍然复用同一套扫描逻辑，只是候选列范围按圆环方向做镜像。
            if((circle_reference_limit_side > 0 && col > circle_reference_limit_col) ||
               (circle_reference_limit_side < 0 && col < circle_reference_limit_col))
            {
                continue;
            }
        }

        // 每隔若干列从下往上扫。
        // 这一列如果能一直保持白到更高的位置，就说明它更可能落在赛道中间。
        for(int row = SEARCH_UVC_HEIGHT - 1; row > SEARCHSTOPROW; row -= CONTRASTOFFSET)
        {
            int compare_row = row - CONTRASTOFFSET;

            if(compare_row < 0)
            {
                compare_row = SEARCHSTOPROW;
            }

            uint8_t current_gray = *(pImage + row * SEARCH_UVC_WIDTH + col);
            uint8_t compare_gray = *(pImage + compare_row * SEARCH_UVC_WIDTH + col);

            // 当前点仍然属于白区，说明这列在这一行还在赛道内部，继续往上找。
            if(vision_is_white(current_gray))
            {
                continue;
            }

            // 当前点已经明显变黑，认为这一列在这里离开白赛道。
            if(vision_is_black(current_gray))
            {
                remote_distance[col_index] = (uint8_t)row;
                break;
            }

            // 不是纯黑但出现明显灰度突变，也视为边界附近。
            if(current_gray <= compare_gray &&
               vision_contrast_ratio(current_gray, compare_gray) > reference_contrast_ratio)
            {
                remote_distance[col_index] = (uint8_t)row;
                break;
            }
        }
    }

    // remote_distance 越小，说明这一列越靠近图像上方才离开白赛道，也就是“看得更远”。
    // 所以这里取最小值对应的列，作为整帧统一的起扫参考列。
    uint8_t reference_col_index = Find_Extreme_value(remote_distance, 0, scan_col_count, 0);
    reference_col = (uint8_t)(reference_col_index * CONTRASTOFFSET);
    reference_col = func_limit_ab(reference_col, 0, SEARCH_UVC_WIDTH - 1);
    reference_point = remote_distance[reference_col_index];
}

//寻找边界并进入视觉结果的结构体
void Search_Boundary(const uint8_t *pImage, VisionResult &result)
{
    // 从参考点下移一个可调偏移量后再开始扫线，减少把最远端分界线误判成边界。
    uint8_t search_start_row = reference_point + REFERENCE_ROW_OFFSET;
    uint8_t search_last_row = SEARCH_UVC_HEIGHT - 1;

    // 边界数组在这里按本帧统一初始化，避免在视觉总入口里整帧清空所有结果。
    vision_prepare_boundary_result(result);

    if(search_start_row > search_last_row)
    {
        // 偏移后如果已经超出图像底部，回退到合法行范围，避免越界或空循环。
        search_start_row = SEARCHSTOPROW;
    }

    // 这里只在“参考行到图像底部”之间搜索。
    // 不再从整屏顶部一路往下扫，避免无效区域影响中线。
    for(int row = search_start_row; row <= search_last_row; row += CONTRASTOFFSET)
    {
        uint8_t left_boundary = TRACK_LEFT_EDGE;
        uint8_t right_boundary = TRACK_RIGHT_EDGE;
        uint8_t left_found = 0;
        uint8_t right_found = 0;

        left_found = vision_find_boundary_on_side(pImage, (uint8_t)row, reference_col, -1, left_boundary);
        right_found = vision_find_boundary_on_side(pImage, (uint8_t)row, reference_col, 1, right_boundary);

        result.left_boundary[row] = left_boundary;
        result.right_boundary[row] = right_boundary;
        result.left_found[row] = left_found;
        result.right_found[row] = right_found;
    }

    result.valid_row_start = search_start_row;
    result.valid_row_end = search_last_row;
}

//十字/大开口检查与补线
void Cross_Check_And_Repair(VisionResult &result)
{
    // 十字补线主流程：
    // 1. 用底部稳定赛道估计本帧正常宽度；
    // 2. 从下往上找第一段“缺边/外扩”的连续异常区；
    // 3. 在异常区内统计左右两侧是否都出现过开口证据；
    // 4. 只有确认是“双侧开口”后，才按十字补线，给后续单边开口元素预留空间。
    const int width_expand = 6;
    const int min_cross_height = 2 * CONTRASTOFFSET;
    const int min_side_expand = 4;
    int row = 0;
    int base_width = 0;
    int base_count = 0;
    int min_cross_width = 0;
    int support_bottom = -1;
    int support_top = -1;
    int abnormal_count = 0;
    int support_left_limit = 0;
    int support_right_limit = 0;
    int left_open_count = 0;
    int right_open_count = 0;
    int width_open_count = 0;

    result.cross_flag = 0;

    // 用底部最近的几条有效边界估计正常赛道宽度，作为十字宽度判定基准。
    for(row = result.valid_row_end; row >= result.valid_row_start && base_count < 3; row -= CONTRASTOFFSET)
    {
        if(result.left_found[row] && result.right_found[row])
        {
            base_width += (int)result.right_boundary[row] - (int)result.left_boundary[row];
            base_count++;
        }
    }

    if(0 == base_count)
    {
        return;
    }

    base_width /= base_count;
    min_cross_width = base_width + width_expand;

    // 从下往上找第一段异常区。
    // 只要连续多行出现“缺边”或“宽度异常”，就认为进入十字/大开口区。
    for(row = result.valid_row_end; row >= result.valid_row_start; row -= CONTRASTOFFSET)
    {
        uint8_t abnormal = vision_row_is_cross_abnormal(result, row, min_cross_width);

        if(0 == abnormal)
        {
            if(abnormal_count >= min_cross_height && support_bottom >= 0)
            {
                support_top = row;
                break;
            }

            abnormal_count = 0;
            support_bottom = row;
            continue;
        }
        abnormal_count++;
    }

    if(abnormal_count < min_cross_height || support_bottom < 0 || support_top < 0)
    {
        return;
    }

    support_left_limit = (result.left_boundary[support_bottom] < result.left_boundary[support_top]) ?
                         result.left_boundary[support_bottom] : result.left_boundary[support_top];
    support_right_limit = (result.right_boundary[support_bottom] > result.right_boundary[support_top]) ?
                          result.right_boundary[support_bottom] : result.right_boundary[support_top];

    // 十字允许斜着进入，所以不要求“同一行同时双边张开”。
    // 但在整段异常区里，左右两侧都必须各自出现过开口证据；
    // 如果始终只有单边异常，就把它留给后续圆环/单边开口逻辑处理。
    for(row = support_top; row <= support_bottom; row += CONTRASTOFFSET)
    {
        int width = (int)result.right_boundary[row] - (int)result.left_boundary[row];
        uint8_t left_open = 0;
        uint8_t right_open = 0;

        if(width >= min_cross_width)
        {
            width_open_count++;
        }

        if(!result.left_found[row] ||
           result.left_boundary[row] <= TRACK_LEFT_EDGE ||
           result.left_boundary[row] <= support_left_limit - min_side_expand)
        {
            left_open = 1;
        }

        if(!result.right_found[row] ||
           result.right_boundary[row] >= TRACK_RIGHT_EDGE ||
           result.right_boundary[row] >= support_right_limit + min_side_expand)
        {
            right_open = 1;
        }

        left_open_count += left_open;
        right_open_count += right_open;
    }

    if(0 == width_open_count || 0 == left_open_count || 0 == right_open_count)
    {
        return;
    }

    // 对左右边界分别做直线补齐，把异常区间内的断点回填成连续边界。
    // 这里用的是异常区前后的稳定支撑行，而不是单帧跳变点，因此对十字更稳。
    connect_point(result.left_boundary, support_bottom, support_top);
    connect_point(result.right_boundary, support_bottom, support_top);

    for(row = support_top; row <= support_bottom; row += CONTRASTOFFSET)
    {
        result.left_found[row] = 1;
        result.right_found[row] = 1;
    }

    result.cross_flag = 1;
}

//中线计算
void Calculate_Center_Line(VisionResult &result)
{
    // 这里只在已经扫过边界的有效行区间内计算中线。
    for(int row = result.valid_row_start; row <= result.valid_row_end; row += CONTRASTOFFSET)
    {
        // 中线直接取左右边界中点。
        int center = (result.left_boundary[row] + result.right_boundary[row]) / 2;

        result.center_line[row] = vision_limit_col(center);
    }

    Calculate_Center_Error(result);
}

// 中线被正常巡线或绕行逻辑生成后，统一在这里更新底盘控制误差。
static void Calculate_Center_Error(VisionResult &result)
{
    result.line_error = 0;
    result.heading_error = 0;

    if(result.valid_row_start > result.valid_row_end)
    {
        result.line_error = 0;
        result.heading_error = 0;
        return;
    }

    // 这里的百分比坐标系固定为：0% 在中线底部，100% 在中线顶部。
    uint8_t near_row_start = vision_row_from_centerline_percent(result, NEAR_PERCENT_END);
    uint8_t near_row_end = vision_row_from_centerline_percent(result, NEAR_PERCENT_START);
    int near_center_average = vision_average_center_line(result,
                                                         near_row_start,
                                                         near_row_end);
    uint8_t far_row_start = vision_row_from_centerline_percent(result, FAR_PERCENT_END);
    uint8_t far_row_end = vision_row_from_centerline_percent(result, FAR_PERCENT_START);
    int far_center_average = vision_average_center_line(result,
                                                        far_row_start,
                                                        far_row_end);

    result.line_error = (int16_t)(near_center_average - (SEARCH_UVC_WIDTH / 2));
    result.heading_error = (float)(far_center_average - near_center_average);
}

static void Apply_Red_Avoidance_Line(VisionResult &result, RedAvoidanceDirection direction)
{
    for(int row = result.valid_row_start; row <= result.valid_row_end; row += CONTRASTOFFSET)
    {
        if(RED_AVOIDANCE_DIRECTION_LEFT == direction)
        {
            result.center_line[row] = vision_limit_col((int)result.left_boundary[row] - RED_AVOIDANCE_OFFSET);
        }
        else if(RED_AVOIDANCE_DIRECTION_RIGHT == direction)
        {
            result.center_line[row] = vision_limit_col((int)result.right_boundary[row] + RED_AVOIDANCE_OFFSET);
        }
    }

    Calculate_Center_Error(result);
}

void Set_Red_Avoidance_Direction(VisionResult &result, RedAvoidanceDirection direction)
{
    result.red_avoidance_direction = (uint8_t)direction;
}

void Red_Avoidance_State_Machine(VisionResult &result)
{
    switch(result.red_avoidance_state)
    {
        case RED_AVOIDANCE_STATE_IDLE:
            result.red_avoidance_direction = RED_AVOIDANCE_DIRECTION_UNKNOWN;
            if(0 == result.red_found)
            {
                return;
            }
            result.red_avoidance_state = RED_AVOIDANCE_STATE_WAIT_DIRECTION;
            [[fallthrough]];

        case RED_AVOIDANCE_STATE_WAIT_DIRECTION:
            if(RED_AVOIDANCE_DIRECTION_LEFT == result.red_avoidance_direction)
            {
                result.red_avoidance_state = RED_AVOIDANCE_STATE_LEFT;
            }
            else if(RED_AVOIDANCE_DIRECTION_RIGHT == result.red_avoidance_direction)
            {
                result.red_avoidance_state = RED_AVOIDANCE_STATE_RIGHT;
            }
            else if(RED_AVOIDANCE_DIRECTION_STRAIGHT == result.red_avoidance_direction)
            {
                result.red_avoidance_state = RED_AVOIDANCE_STATE_STRAIGHT;
            }
            break;

        case RED_AVOIDANCE_STATE_LEFT:
        case RED_AVOIDANCE_STATE_RIGHT:
        case RED_AVOIDANCE_STATE_STRAIGHT:
        case RED_AVOIDANCE_STATE_RETURN_TRACK:
            break;

        default:
            Reset_Red_Avoidance(result);
            return;
    }

    if(RED_AVOIDANCE_STATE_LEFT == result.red_avoidance_state)
    {
        Apply_Red_Avoidance_Line(result, RED_AVOIDANCE_DIRECTION_LEFT);
    }
    else if(RED_AVOIDANCE_STATE_RIGHT == result.red_avoidance_state)
    {
        Apply_Red_Avoidance_Line(result, RED_AVOIDANCE_DIRECTION_RIGHT);
    }
}

void Start_Red_Avoidance_Return(VisionResult &result)
{
    result.red_avoidance_state = RED_AVOIDANCE_STATE_RETURN_TRACK;
    Calculate_Center_Line(result);
}

void Reset_Red_Avoidance(VisionResult &result)
{
    result.red_avoidance_state = RED_AVOIDANCE_STATE_IDLE;
    result.red_avoidance_direction = RED_AVOIDANCE_DIRECTION_UNKNOWN;
}

// 红色标识纸检测：赛道只有蓝白，红纸是纯红水平长方形，因此只做颜色阈值和几何筛选。
static void Detect_Red_Marker(const cv::Mat &color_frame, VisionResult &result)
{
    result.red_found = 0;
    result.red_x = 0;
    result.red_y = 0;
    result.red_w = 0;
    result.red_h = 0;
    result.image_found = 0;
    result.image_x = 0;
    result.image_y = 0;
    result.image_w = 0;
    result.image_h = 0;
    for(uint8_t index = 0; index < 4; index++)
    {
        result.red_corner_x[index] = 0;
        result.red_corner_y[index] = 0;
        result.image_corner_x[index] = 0;
        result.image_corner_y[index] = 0;
    }

    if(color_frame.empty())
    {
        return;
    }

    const cv::Mat &frame = color_frame;
    const float result_scale_x = static_cast<float>(SEARCH_UVC_WIDTH) /
                                 static_cast<float>(frame.cols);
    const float result_scale_y = static_cast<float>(SEARCH_UVC_HEIGHT) /
                                 static_cast<float>(frame.rows);
    // 行号越大越靠近画面底部：至少跳过顶部固定区域，
    // 最长白列上端更低时，则从该行开始检测红色。
    const int detect_top_row =
        (result.reference_row > RED_MARKER_DETECT_TOP_ROW) ?
        result.reference_row : RED_MARKER_DETECT_TOP_ROW;

    cv::Mat hsv, mask_low, mask_high, red_mask;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv,
                cv::Scalar(RED_HUE_LOW_MIN, RED_SATURATION_MIN, RED_BRIGHTNESS_MIN),
                cv::Scalar(RED_HUE_LOW_MAX, RED_HSV_CHANNEL_MAX, RED_HSV_CHANNEL_MAX),
                mask_low);
    cv::inRange(hsv,
                cv::Scalar(RED_HUE_HIGH_MIN, RED_SATURATION_MIN, RED_BRIGHTNESS_MIN),
                cv::Scalar(RED_HUE_HIGH_MAX, RED_HSV_CHANNEL_MAX, RED_HSV_CHANNEL_MAX),
                mask_high);
    cv::bitwise_or(mask_low, mask_high, red_mask);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(red_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    int best_index = -1;
    int best_bottom = -1;
    double best_area = 0.0;

    for(size_t index = 0; index < contours.size(); index++)
    {
        cv::Rect box = cv::boundingRect(contours[index]);
        const cv::RotatedRect rotated_box = cv::minAreaRect(contours[index]);
        const double area = (double)cv::countNonZero(red_mask(box));
        // minAreaRect 的尺寸按像素中心计算，加 1 后与原先的像素外接框尺寸保持一致。
        const float rotated_side_a = rotated_box.size.width + 1.0f;
        const float rotated_side_b = rotated_box.size.height + 1.0f;
        const float long_side = (rotated_side_a >= rotated_side_b) ?
                                rotated_side_a : rotated_side_b;
        const float short_side = (rotated_side_a < rotated_side_b) ?
                                 rotated_side_a : rotated_side_b;
        float aspect = (short_side <= 0.0f) ? 0.0f : (long_side / short_side);
        int bottom = box.y + box.height;
        const int result_left = cvRound(box.x * result_scale_x);
        const int result_right = cvRound((box.x + box.width) * result_scale_x);
        const int result_top = cvRound(box.y * result_scale_y);
        const int result_bottom = cvRound(bottom * result_scale_y);
        // const int result_center_col = func_limit_ab(
        //     cvRound((box.x + box.width / 2.0f) * result_scale_x),
        //     0,
        //     SEARCH_UVC_WIDTH - 1);
        // const int result_center_row = func_limit_ab(
        //     cvRound((box.y + box.height / 2.0f) * result_scale_y),
        //     0,
        //     SEARCH_UVC_HEIGHT - 1);

        if(result_left < RED_MARKER_DETECT_LEFT_MARGIN ||
           result_right > SEARCH_UVC_WIDTH - RED_MARKER_DETECT_RIGHT_MARGIN ||
           result_top < detect_top_row ||
           result_bottom > SEARCH_UVC_HEIGHT - RED_MARKER_DETECT_BOTTOM_MARGIN)
        {
            continue;
        }

        if(long_side < RED_MARKER_MIN_LONG_SIDE ||
           short_side < RED_MARKER_MIN_SHORT_SIDE ||
           area < RED_MARKER_MIN_AREA ||
           area > RED_MARKER_MAX_AREA)
        {
            continue;
        }
        if(aspect < RED_MARKER_MIN_ASPECT || aspect > RED_MARKER_MAX_ASPECT)
        {
            continue;
        }

        // 暂时禁用赛道中心筛选：红色标靶会干扰同一行的左右边界，
        // 使真实标靶被误判为偏离赛道中心。
        // const int track_center_col =
        //     ((int)result.left_boundary[result_center_row] +
        //      (int)result.right_boundary[result_center_row]) / 2;
        // const int center_offset = result_center_col - track_center_col;
        // if(center_offset < -RED_MARKER_CENTER_TOLERANCE ||
        //    center_offset > RED_MARKER_CENTER_TOLERANCE)
        // {
        //     continue;
        // }

        if(bottom > best_bottom || (bottom == best_bottom && area > best_area))
        {
            best_index = (int)index;
            best_bottom = bottom;
            best_area = area;
        }
    }

    if(best_index < 0)
    {
        return;
    }

    cv::Rect red_box = cv::boundingRect(contours[(size_t)best_index]);
    const int red_x = func_limit_ab(cvRound(red_box.x * result_scale_x),
                                    0,
                                    SEARCH_UVC_WIDTH - 1);
    const int red_y = func_limit_ab(cvRound(red_box.y * result_scale_y),
                                    0,
                                    SEARCH_UVC_HEIGHT - 1);
    const int red_right = func_limit_ab(
        cvRound((red_box.x + red_box.width) * result_scale_x),
        red_x + 1,
        SEARCH_UVC_WIDTH);
    const int red_bottom = func_limit_ab(
        cvRound((red_box.y + red_box.height) * result_scale_y),
        red_y + 1,
        SEARCH_UVC_HEIGHT);
    const int red_w = red_right - red_x;
    const int red_h = red_bottom - red_y;

    result.red_found = 1;
    result.red_x = (uint8_t)red_x;
    result.red_y = (uint8_t)red_y;
    result.red_w = (uint8_t)red_w;
    result.red_h = (uint8_t)red_h;

    result.red_corner_x[0] = (uint8_t)red_x;
    result.red_corner_y[0] = (uint8_t)red_y;
    result.red_corner_x[1] = (uint8_t)(red_right - 1);
    result.red_corner_y[1] = (uint8_t)red_y;
    result.red_corner_x[2] = (uint8_t)(red_right - 1);
    result.red_corner_y[2] = (uint8_t)(red_bottom - 1);
    result.red_corner_x[3] = (uint8_t)red_x;
    result.red_corner_y[3] = (uint8_t)(red_bottom - 1);

    const int red_center_x = red_x + red_w / 2;
    const int red_bottom_y = red_y + red_h;
    const int image_x = func_limit_ab(red_center_x - IMAGE_ROI_SIZE / 2,
                                      0,
                                      SEARCH_UVC_WIDTH - IMAGE_ROI_SIZE);
    const int image_y = func_limit_ab(red_bottom_y - IMAGE_ROI_SIZE,
                                      0,
                                      SEARCH_UVC_HEIGHT - IMAGE_ROI_SIZE);
    const int image_right = image_x + IMAGE_ROI_SIZE - 1;
    const int image_bottom = image_y + IMAGE_ROI_SIZE - 1;

    result.image_found = 1;
    result.image_x = (uint8_t)image_x;
    result.image_y = (uint8_t)image_y;
    result.image_w = IMAGE_ROI_SIZE;
    result.image_h = IMAGE_ROI_SIZE;

    result.image_corner_x[0] = (uint8_t)image_x;
    result.image_corner_y[0] = (uint8_t)image_y;
    result.image_corner_x[1] = (uint8_t)image_right;
    result.image_corner_y[1] = (uint8_t)image_y;
    result.image_corner_x[2] = (uint8_t)image_right;
    result.image_corner_y[2] = (uint8_t)image_bottom;
    result.image_corner_x[3] = (uint8_t)image_x;
    result.image_corner_y[3] = (uint8_t)image_bottom;
}

//视觉进程
void Process_Vision(const uint8_t *pImage, VisionResult &result)
{
    // 没拿到图像时直接输出默认结果，并标记为丢线。
    if(nullptr == pImage)
    {
        vision_reset_result_when_lost(result);
        return;
    }

    // 1. 绕行期间保持触发帧的参考值；回归时只在底部亮度恢复到锁定值附近后才更新。
    if(RED_AVOIDANCE_STATE_IDLE == result.red_avoidance_state)
    {
        Get_Reference_Point(pImage);
    }
    else if(RED_AVOIDANCE_STATE_RETURN_TRACK == result.red_avoidance_state &&
            Red_Return_Reference_Matched(pImage))
    {
        Reset_Red_Avoidance(result);
        Get_Reference_Point(pImage);
    }
    // 2. 找到最适合起扫的参考列和参考行。
    Get_Reference_Col(pImage);
    // 3. 从参考列向左右搜索各行边界。
    Search_Boundary(pImage, result);

    // 4. 检查是否进入十字,并做相应补线。
   // Cross_Check_And_Repair(result);
    // 5. 检查是否进入圆环,并做相应补线。
   Round_Check_And_Repair(result);

    // 6. 计算中线、丢线状态和控制误差。
    Calculate_Center_Line(result);
    // 把本帧内部算出来的参考信息写回结果结构体，方便显示和调试。
    result.reference_col = reference_col;
    result.reference_row = reference_point;
}

// 后续如果你要做异帧处理,视觉模型识别判断的函数接口
void Process_Vision_Frame(const uint8_t *pImage,
                          const cv::Mat &color_frame,
                          VisionResult &result)
{
    Detect_Red_Marker(color_frame, result);
    Process_Vision(pImage, result);
    Red_Avoidance_State_Machine(result);
}
