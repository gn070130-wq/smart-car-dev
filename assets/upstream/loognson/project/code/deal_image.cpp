#include "deal_image.h"

// image tracing
uint8_t want_start_row = WANT_START_ROW;
uint8_t want_end_row = WANT_END_ROW;
uint8_t longest_white_column1;
uint8_t longest_white_row1;
uint8_t longest_white_column2;
uint8_t longest_white_row2;
// 十字路口
struct crossroad judge_crossroad =
{
    .cross_flag = false,
    .cross_left_flag = false,
    .cross_right_flag = false,
    .left_blank_height = 0,
    .right_blank_height = 0
};
// 斑马线
struct zebra_line judge_zebra_line =
{
    .flag = 0
};
// 避障
struct roadblock judge_roadblock =
{
    .left_roadblock_flag = false,
    .right_roadblock_flag = false, 
    .left_line = 0,
    .right_line = 0,
    .encoder_count = 0
};
// 坡道
struct ramp judge_ramp =
{
    .flag = false,
    .left_width = 0,
    .right_width = 0,
    .encoder_count = 0
};
uint8_t road_differ[120] =
{
/*   0 ~ 9   */     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*  10 ~ 19  */     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*  20 ~ 29  */     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*  30 ~ 39  */   156,  21,  23,  24,  26,  28,  29,  30,  31,  33,
/*  40 ~ 49  */    35,  37,  38,  40,  41,  42,  44,  45,  47,  48,
/*  50 ~ 59  */    49,  51,  52,  54,  56,  57,  58,  60,  61,  63,
/*  60 ~ 69  */    64,  65,  67,  68,  70,  72,  73,  74,  75,  77,
/*  70 ~ 79  */    79,  80,  82,  83,  84,  86,  88,  89,  90,  92,
/*  80 ~ 89  */    93,  95,  97,  98,  99, 100, 102, 104, 105, 106, 
/*  90 ~ 99  */   107, 108, 109, 111, 112, 114, 115, 116, 117, 119,
/* 100 ~ 109 */   120, 121, 123, 125, 127, 128, 129, 131, 132, 149,
/* 110 ~ 119 */   156,   0,   0,   0,   0,   0,   0,   0,   0,   0
};
// 环岛
struct island judge_island =
{
    .range_row = 10,
    .range_column = 15,
    .out_island_flag = false,
    .left_island_type = 0,
    .left_type1_flag = false,
    .left_type2_k = 0.0,
    .left_type2_b = 0.0,
    .left_type4_k = 0.0,
    .left_type4_b = 0.0,
    .right_island_type = 0,
    .right_type1_flag = false,
    .right_type2_k = 0.0,
    .right_type2_b = 0.0,
    .right_type4_k = 0.0,
    .right_type4_b = 0.0
};
// 元素冷却
struct cooldown element_cooldown =
{
    .roadblock_cooldown = 0,
    .ramp_cooldown = 0,
    .island_cooldown = 0
};
// 元素状态变量，用于跟踪上一次的状态
static bool last_cross_flag = false;
static bool last_cross_left_flag = false;
static bool last_cross_right_flag = false;
static uint8_t last_left_island_type = 0;
static uint8_t last_right_island_type = 0;
static uint8_t last_zebra_line_flag = 0;
static bool last_left_roadblock_flag = false;
static bool last_right_roadblock_flag = false;
static bool last_ramp_flag = false;

/****************************** image tracing ******************************/
/**
 * @brief 初始化边界搜索
 * @param void
 * @return void
 * @note 初始化边界搜索的参数，包括边界搜索的起始行、结束行、边界搜索的预测列、边界搜索的预测行
 * @author YuYu
 */
void search_edge_init(void)
{
    // find first point
    verge.left_first_point.row = WANT_START_ROW;
    verge.right_first_point.row = WANT_START_ROW;
    verge.left_first_point.column = LEFT_BOUNDARY_COLUMN;
    verge.right_first_point.column = RIGHT_BOUNDARY_COLUMN;
    verge.left_first_point.flag = 0;
    verge.right_first_point.flag = 0;
    // find last point
    verge.left_last_point.row = WANT_START_ROW;
    verge.right_last_point.row = WANT_START_ROW;
    verge.left_last_point.column = LEFT_BOUNDARY_COLUMN;
    verge.right_last_point.column = RIGHT_BOUNDARY_COLUMN;
    verge.left_last_point.flag = 0;
    verge.right_last_point.flag = 0;
    // middle line
    center.middle_end_row = WANT_START_ROW;
    // inflection point
    verge.left_inflection.row = WANT_END_ROW;
    verge.right_inflection.row = WANT_END_ROW;
    verge.left_inflection.column = LEFT_BOUNDARY_COLUMN;
    verge.right_inflection.column = RIGHT_BOUNDARY_COLUMN;
    verge.left_inflection.flag = 0;
    verge.right_inflection.flag = 0;
    // lose line
    verge.left_lose_line.row = WANT_END_ROW;
    verge.right_lose_line.row = WANT_END_ROW;
    verge.left_lose_line.column = LEFT_BOUNDARY_COLUMN;
    verge.right_lose_line.column = RIGHT_BOUNDARY_COLUMN;
    verge.left_lose_line.flag = 0;
    verge.right_lose_line.flag = 0;
    // longest white column
    longest_white_column.column = START_MIDDLE_COLUMN;
    longest_white_column.row = WANT_START_ROW;
    longest_white_column1 = START_MIDDLE_COLUMN;
    longest_white_row1 = WANT_START_ROW;
    longest_white_column2 = START_MIDDLE_COLUMN;
    longest_white_row2 = WANT_START_ROW;
    // clean verge
    for(uint8_t row = WANT_START_ROW; row >= WANT_END_ROW; row--)
    {
        verge.left[row].column = LEFT_BOUNDARY_COLUMN;
        verge.left[row].flag = 0;
        verge.right[row].column = RIGHT_BOUNDARY_COLUMN;
        verge.right[row].flag = 0;
        center.middle[row].column = START_MIDDLE_COLUMN;
        center.middle[row].flag = 0;
    }
}

/**
 * @brief 搜索图像边界
 * @param void
 * @return void
 * @note 搜索图像边界，包括搜索最长白列、搜索中线、搜索偏差
 * @author YuYu
 */
void search_image_edge(void)
{
    // init image data
    uint8_t lrow, lcolumn;
    float lk, lb;
    int16_t lx0, lx1, ly0, ly1;
    uint8_t rrow, rcolumn;
    float rk, rb;
    int16_t rx0, rx1, ry0, ry1;
    uint8_t forecast_column;
    // longest white first column
    uint8_t longest_left_column;
    uint8_t longest_right_column;
    // lose point
    uint8_t left_point_count = 0;
    uint8_t right_point_count = 0;
    uint8_t left_lose_count = 0;
    uint8_t right_lose_count = 0;
    search_edge_init();
    /**************************************** init special edge case ****************************************/
    for(lcolumn = RIGHT_BOUNDARY_COLUMN - 8; lcolumn > START_MIDDLE_COLUMN - 10; lcolumn--)
    {
        if(image_buffer[want_start_row][lcolumn] == WHITE && image_buffer[want_start_row][lcolumn - 1] == BLACK
                && image_buffer[want_start_row][lcolumn - 2] == BLACK && image_buffer[want_start_row][lcolumn - 3] == BLACK
                && image_buffer[want_start_row][lcolumn - 20] == BLACK && image_buffer[want_start_row][lcolumn - 30] == BLACK
                && image_buffer[(want_start_row - 5)][lcolumn - 2] == BLACK)
        {
            verge.left_first_point.column = lcolumn + 20;
            verge.left_first_point.column = limit_value(verge.left_first_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            break;
        }
    }
    for(rcolumn = LEFT_BOUNDARY_COLUMN + 8; rcolumn < START_MIDDLE_COLUMN + 10; rcolumn++)
    {
        if(image_buffer[want_start_row][rcolumn] == WHITE && image_buffer[want_start_row][rcolumn + 1] == BLACK
                && image_buffer[want_start_row][rcolumn + 2] == BLACK && image_buffer[want_start_row][rcolumn + 3] == BLACK
                && image_buffer[want_start_row][rcolumn + 20] == BLACK && image_buffer[want_start_row][rcolumn + 30] == BLACK
                && image_buffer[(want_start_row - 5)][rcolumn + 2] == BLACK)
        {
            verge.right_first_point.column = rcolumn - 20;
            verge.right_first_point.column = limit_value(verge.right_first_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            break;
        }
    }
    if(verge.left_first_point.column == LEFT_BOUNDARY_COLUMN)
    {
        verge.left_first_point.column = START_MIDDLE_COLUMN - 25; // 在中间偏左
        verge.left_first_point.column = limit_value(verge.left_first_point.column, START_MIDDLE_COLUMN, LEFT_BOUNDARY_COLUMN);
    }
    if(verge.right_first_point.column == RIGHT_BOUNDARY_COLUMN)
    {
        verge.right_first_point.column = START_MIDDLE_COLUMN + 25; // 在中间偏右
        verge.right_first_point.column = limit_value(verge.right_first_point.column, RIGHT_BOUNDARY_COLUMN, START_MIDDLE_COLUMN);
    }
    /**************************************** find longest white line ****************************************/
    search_left_edge(WANT_START_ROW, verge.left_first_point.column, LEFT_BOUNDARY_COLUMN);
    search_right_edge(WANT_START_ROW, verge.right_first_point.column, RIGHT_BOUNDARY_COLUMN);
    // 限制寻找最长白列的范围
    if(verge.left[WANT_START_ROW].column < (LEFT_BOUNDARY_COLUMN + 8)) // MT9V03X_W*5% ( 160 * 5% = 8 )
    {
        longest_right_column = LEFT_BOUNDARY_COLUMN;
    }
    else
    {
        longest_right_column = ((verge.right[WANT_START_ROW].column + verge.left[WANT_START_ROW].column) / 4 * 1);
    }
    if(verge.right[WANT_START_ROW].column > (RIGHT_BOUNDARY_COLUMN - 8)) // MT9V03X_W*5% ( 160 * 5% = 8 )
    {
        longest_left_column = RIGHT_BOUNDARY_COLUMN;
    }
    else
    {
        longest_left_column = ((verge.right[WANT_START_ROW].column + verge.left[WANT_START_ROW].column) / 4 * 3);
    }
    // 寻找最长白列
    for(lcolumn = verge.left[WANT_START_ROW].column; lcolumn + 5 < longest_left_column; lcolumn = lcolumn + 5)
    {
        search_longest_row(WANT_START_ROW, WANT_END_ROW, lcolumn);
        if(longest_white_column.down[lcolumn].row < longest_white_row1 && longest_white_column.down[lcolumn].row >= WANT_END_ROW)
        {
            longest_white_column1 = lcolumn;
            longest_white_row1 = longest_white_column.down[lcolumn].row;
        }
    }
    for(rcolumn = longest_right_column; rcolumn + 5 < verge.right[WANT_START_ROW].column; rcolumn = rcolumn + 5)
    {
        search_longest_row(WANT_START_ROW, WANT_END_ROW, rcolumn);
        if(longest_white_column.down[rcolumn].row < longest_white_row2 && longest_white_column.down[rcolumn].row >= WANT_END_ROW)
        {
            longest_white_column2 = rcolumn;
            longest_white_row2 = longest_white_column.down[rcolumn].row;
        }
    }
    longest_white_column.column = (uint8_t)(((longest_white_column1 + longest_white_column2)/2.0) + 0.5f); // 保证四舍五入之后为正确的最长白列
    longest_white_column.row = (uint8_t)find_min(longest_white_row1, longest_white_row2);
    // 只在找到有效白列时才更新起始行
    if(longest_white_column.column >= LEFT_BOUNDARY_COLUMN && longest_white_column.column <= RIGHT_BOUNDARY_COLUMN 
        && longest_white_column.row <= WANT_START_ROW && longest_white_column.row >= WANT_END_ROW)
    {
        want_end_row = longest_white_column.row;
        verge.left_first_point.column = longest_white_column.column;
        verge.right_first_point.column = longest_white_column.column;
        if(want_end_row < WANT_END_ROW)
        {
            want_end_row = WANT_END_ROW;
        }
        if(want_end_row > LONGEST_END_ROW)
        {
            want_end_row = LONGEST_END_ROW;
        }
    }
    /**************************************** find left edge <-- ****************************************/
    search_left_edge(want_start_row, verge.left_first_point.column, LEFT_BOUNDARY_COLUMN);
    for(lrow = (want_start_row - 1); lrow >= want_end_row; lrow--)
    {
        search_left_edge(lrow, verge.left_first_point.column, LEFT_BOUNDARY_COLUMN);
        if(verge.left[lrow].flag == 1)
        {
            left_point_count++;
        }
        else
        {
            left_point_count = 0;
        }
        if(abs(verge.left[lrow + 1].column - verge.left[lrow].column) >= 8)
        {
            left_point_count = 0;
        }
        if(left_point_count >= 7)
        {
            verge.left_first_point.flag = 1;
            verge.left_first_point.row = lrow + 3;
            break;
        }
    }
    if(verge.left_first_point.flag)
    {
        verge.left_last_point.row = want_end_row;
        for(lrow = (verge.left_first_point.row - 3); lrow >= want_end_row; lrow--)
        {
            forecast_column = verge.left[lrow + 1].column + (verge.left[lrow + 1].column - verge.left[lrow + 3].column)/2;
            forecast_column = limit_value(forecast_column, (RIGHT_BOUNDARY_COLUMN - RANGE_COLUMN), (LEFT_BOUNDARY_COLUMN + RANGE_COLUMN));
            search_left_edge(lrow, (forecast_column + RANGE_COLUMN), (forecast_column - RANGE_COLUMN));
            if(verge.left[lrow].flag == 0)
            {
                left_lose_count++;
                verge.left[lrow].column = verge.left[lrow + 1].column + (verge.left[lrow + 1].column - verge.left[lrow + 3].column)/2;
                verge.left[lrow].column = limit_value(verge.left[lrow].column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                verge.left[lrow].flag = 1;
            }
            else
            {
                left_lose_count = 0;
            }
            if(left_lose_count >= 4 && lrow > FAR_SEARCH_ROW)
            {
                verge.left[lrow].flag = 0;
                verge.left[lrow + 1].flag = 0;
                verge.left[lrow + 2].flag = 0;
                verge.left[lrow + 3].flag = 0;
                verge.left[lrow].column = LEFT_BOUNDARY_COLUMN;
                verge.left[lrow + 1].column = LEFT_BOUNDARY_COLUMN;
                verge.left[lrow + 2].column = LEFT_BOUNDARY_COLUMN;
                verge.left[lrow + 3].column = LEFT_BOUNDARY_COLUMN;
                verge.left_lose_line.row = lrow + 4;
                verge.left_last_point.row = lrow + 4;
                verge.left_lose_line.flag = 1;
                break;
            }
            else if(left_lose_count >= 2 && lrow <= NEAR_SEARCH_ROW)
            {
                verge.left[lrow].flag = 0;
                verge.left[lrow + 1].flag = 0;
                verge.left[lrow].column = LEFT_BOUNDARY_COLUMN;
                verge.left[lrow + 1].column = LEFT_BOUNDARY_COLUMN;
                verge.left_lose_line.row = lrow + 2;
                verge.left_last_point.row = lrow + 2;
                verge.left_lose_line.flag = 1;
                break;
            }
            else if(left_lose_count >= 3 && lrow <= FAR_SEARCH_ROW)
            {
                verge.left[lrow].flag = 0;
                verge.left[lrow + 1].flag = 0;
                verge.left[lrow + 2].flag = 0;
                verge.left[lrow].column = LEFT_BOUNDARY_COLUMN;
                verge.left[lrow + 1].column = LEFT_BOUNDARY_COLUMN;
                verge.left[lrow + 2].column = LEFT_BOUNDARY_COLUMN;
                verge.left_lose_line.row = lrow + 3;
                verge.left_last_point.row = lrow + 3;
                verge.left_lose_line.flag = 1;
                break;
            }
        }
        if(verge.left_first_point.row < (want_start_row - 3))
        {
            lx0 = want_start_row;
            ly0 = LEFT_BOUNDARY_COLUMN;
            lx1 = verge.left_first_point.row;
            ly1 = verge.left[lx1].column;
            lk = (float)(1.0 * (ly1 - ly0) / (lx1 - lx0));
            lb = (float)(ly1 - lk * lx1);
            for(lrow = (verge.left_first_point.row + 3); lrow <= want_start_row; lrow++)
            {
                verge.left[lrow].column = (uint8_t)add_line(lrow, lk, lb);
                verge.left[lrow].flag = 1;
                if(verge.left[lrow].column < LEFT_BOUNDARY_COLUMN)
                {
                    verge.left[lrow].column = LEFT_BOUNDARY_COLUMN;
                }
                else if(verge.left[lrow].column > RIGHT_BOUNDARY_COLUMN)
                {
                    verge.left[lrow].column = RIGHT_BOUNDARY_COLUMN;
                }
            }
        }
    }
    else
    {
        verge.left_last_point.row = want_start_row;
    }
    /**************************************** find right edge --> ****************************************/
    search_right_edge(want_start_row, verge.right_first_point.column, RIGHT_BOUNDARY_COLUMN);
    for(rrow = (want_start_row - 1); rrow >= want_end_row; rrow--)
    {
        search_right_edge(rrow, verge.right_first_point.column, RIGHT_BOUNDARY_COLUMN);
        if(verge.right[rrow].flag == 1)
        {
            right_point_count++;
        }
        else
        {
            right_point_count = 0;
        }
        if(abs(verge.right[rrow].column - verge.right[rrow + 1].column) >= 8)
        {
            right_point_count = 0;
        }
        if(right_point_count >= 7)
        {
            verge.right_first_point.flag = 1;
            verge.right_first_point.row = rrow + 3;
            break;
        }
    }
    if(verge.right_first_point.flag)
    {
        verge.right_last_point.row = want_end_row;
        for(rrow = verge.right_first_point.row - 3; rrow >= want_end_row; rrow--)
        {
            forecast_column = verge.right[rrow + 1].column + (verge.right[rrow + 1].column - verge.right[rrow + 3].column)/2;
            forecast_column = limit_value(forecast_column, (RIGHT_BOUNDARY_COLUMN - RANGE_COLUMN), (LEFT_BOUNDARY_COLUMN + RANGE_COLUMN));
            search_right_edge(rrow, (forecast_column - RANGE_COLUMN), (forecast_column + RANGE_COLUMN));
            if(verge.right[rrow].flag == 0)
            {
                right_lose_count++;
                verge.right[rrow].column = verge.right[rrow + 1].column + (verge.right[rrow + 1].column - verge.right[rrow + 3].column)/2;
                verge.right[rrow].column = limit_value(verge.right[rrow].column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                verge.right[rrow].flag = 1;
            }
            else
            {
                right_lose_count = 0;
            }
            if(right_lose_count >= 4 && rrow > FAR_SEARCH_ROW)
            {
                verge.right[rrow].flag = 0;
                verge.right[rrow + 1].flag = 0;
                verge.right[rrow + 2].flag = 0;
                verge.right[rrow + 3].flag = 0;
                verge.right[rrow].column = RIGHT_BOUNDARY_COLUMN;
                verge.right[rrow + 1].column = RIGHT_BOUNDARY_COLUMN;
                verge.right[rrow + 2].column = RIGHT_BOUNDARY_COLUMN;
                verge.right[rrow + 3].column = RIGHT_BOUNDARY_COLUMN;
                verge.right_lose_line.row = rrow + 4;
                verge.right_last_point.row = rrow + 4;
                verge.right_lose_line.flag = 1;
                break;
            }
            else if(right_lose_count >= 2 && rrow <= NEAR_SEARCH_ROW)
            {
                verge.right[rrow].flag = 0;
                verge.right[rrow + 1].flag = 0;
                verge.right[rrow].column = RIGHT_BOUNDARY_COLUMN;
                verge.right[rrow + 1].column = RIGHT_BOUNDARY_COLUMN;
                verge.right_lose_line.row = rrow + 2;
                verge.right_last_point.row = rrow + 2;
                verge.right_lose_line.flag = 1;
                break;
            }
            else if(right_lose_count >= 3 && rrow <= FAR_SEARCH_ROW)
            {
                verge.right[rrow].flag = 0;
                verge.right[rrow + 1].flag = 0;
                verge.right[rrow + 2].flag = 0;
                verge.right[rrow].column = RIGHT_BOUNDARY_COLUMN;
                verge.right[rrow + 1].column = RIGHT_BOUNDARY_COLUMN;
                verge.right[rrow + 2].column = RIGHT_BOUNDARY_COLUMN;
                verge.right_lose_line.row = rrow + 3;
                verge.right_last_point.row = rrow + 3;
                verge.right_lose_line.flag = 1;
                break;
            }
        }
        if(verge.right_first_point.row < (want_start_row - 3))
        {
            rx0 = want_start_row;
            ry0 = RIGHT_BOUNDARY_COLUMN;
            rx1 = verge.right_first_point.row;
            ry1 = verge.right[rx1].column;
            rk = (float)(1.0 * (ry1 - ry0) / (rx1 - rx0));
            rb = (float)(ry1 - rk * rx1);
            for(rrow = (verge.right_first_point.row + 3); rrow <= want_start_row; rrow++)
            {
                verge.right[rrow].column = (uint8_t)add_line(rrow, rk, rb);
                verge.right[rrow].flag = 1;
                if(verge.right[rrow].column > RIGHT_BOUNDARY_COLUMN)
                {
                    verge.right[rrow].column = RIGHT_BOUNDARY_COLUMN;
                }
                else if(verge.right[rrow].column < LEFT_BOUNDARY_COLUMN)
                {
                    verge.right[rrow].column = LEFT_BOUNDARY_COLUMN;
                }
            }
        }
    }
    else
    {
        verge.right_last_point.row = want_start_row;
    }
    /**************************************** search line error ****************************************/
    search_line_error();
    /**************************************** find inflection ****************************************/
    search_left_inflection(verge.left_first_point.row, verge.left_last_point.row);
    search_right_inflection(verge.right_first_point.row, verge.right_last_point.row);
    /**************************************** judge element ****************************************/
    judge_element();
}

/**
 * @brief 搜索线错误
 * @param void
 * @return void
 * @note 搜索线错误，包括搜索左边界线错误、搜索右边界线错误
 * @author YuYu
 */
void search_line_error(void)
{
    for(uint8_t lrow = verge.left_first_point.row; lrow >= want_end_row; lrow--)
    {
        if(verge.left[lrow].column > (longest_white_column.column + 20))
        {
            verge.left[lrow].column = LEFT_BOUNDARY_COLUMN;
            verge.left[lrow].flag = 0;
        }
    }
    for(uint8_t rrow = verge.right_first_point.row; rrow >= want_end_row; rrow--)
    {
        if(verge.right[rrow].column < (longest_white_column.column - 20))
        {
            verge.right[rrow].column = RIGHT_BOUNDARY_COLUMN;
            verge.right[rrow].flag = 0;
        }
    }
    // point left_start_point = {verge.left[verge.left_first_point.row].column, verge.left_first_point.row}; // 左边界起点
    // point left_end_point = {verge.left[verge.left_last_point.row].column, verge.left_last_point.row}; // 左边界终点
    // point right_start_point = {verge.right[verge.right_first_point.row].column, verge.right_first_point.row}; // 右边界起点
    // point right_end_point = {verge.right[verge.right_last_point.row].column, verge.right_last_point.row}; // 右边界终点
    // // 搜索左边界线错误
    // if(angle_line.differ_far < 0 && judge_intersect(left_start_point, left_end_point, right_start_point, right_end_point))
    // {
    //     verge.right_last_point.row = WANT_START_ROW;
    //     for(uint8_t row = WANT_START_ROW; row >= WANT_END_ROW; row--)
    //     {
    //         verge.right[row].column = RIGHT_BOUNDARY_COLUMN;
    //         verge.right[row].flag = 0;
    //     }
    // }
    // // 搜索右边界线错误
    // if(angle_line.differ_far > 0 && judge_intersect(left_start_point, left_end_point, right_start_point, right_end_point))
    // {
    //     verge.left_last_point.row = WANT_START_ROW;
    //     for(uint8_t row = WANT_START_ROW; row >= WANT_END_ROW; row--)
    //     {
    //         verge.left[row].column = LEFT_BOUNDARY_COLUMN;
    //         verge.left[row].flag = 0;
    //     }
    // }
}

/****************************** image tracing ******************************/

/****************************** function ******************************/
/**
 * @brief 计算叉积 (p2-p1)×(p3-p1)
 * @param point p1 点p1
 * @param point p2 点p2
 * @param point p3 点p3
 * @return int16_t 叉积值
 * @note 计算叉积值，用于判断点p3是否在线段p1p2上
 * @author YuYu
 */
int16_t calculate_cross_product(point p1, point p2, point p3)
{
    return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
}

/**
 * @brief 判断点p3是否在线段p1p2上
 * @param point p1 点p1
 * @param point p2 点p2
 * @param point p3 点p3
 * @return bool 是否在线段p1p2上
 * @note 判断点p3是否在线段p1p2上，用于判断点p3是否在线段p1p2上
 * @author YuYu
 */
bool judge_on_segment(point p1, point p2, point p3)
{
    return (p3.x <= find_max(p1.x, p2.x) && p3.x >= find_min(p1.x, p2.x) && p3.y <= find_max(p1.y, p2.y) && p3.y >= find_min(p1.y, p2.y));
}

/**
 * @brief 判断两条线段是否相交
 * @param point left_start_point 左边界起点
 * @param point left_end_point 左边界终点
 * @param point right_start_point 右边界起点
 * @param point right_end_point 右边界终点
 * @return bool 是否相交
 * @note 判断两条线段是否相交，用于判断两条线段是否相交
 * @author YuYu
 */
bool judge_intersect(point left_start_point, point left_end_point, point right_start_point, point right_end_point)
{
    // 计算四个方向值
    int16_t d1 = calculate_cross_product(left_start_point, left_end_point, right_start_point);
    int16_t d2 = calculate_cross_product(left_start_point, left_end_point, right_end_point);
    int16_t d3 = calculate_cross_product(right_start_point, right_end_point, left_start_point);
    int16_t d4 = calculate_cross_product(right_start_point, right_end_point, left_end_point);
    // 一般情况：跨立相交
    if(((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
    {
        return true;
    }
    // 处理共线情况
    if(d1 == 0 && judge_on_segment(left_start_point, left_end_point, right_start_point))
    {
        return true;
    }
    if(d2 == 0 && judge_on_segment(left_start_point, left_end_point, right_end_point))
    {
        return true;
    }
    if(d3 == 0 && judge_on_segment(right_start_point, right_end_point, left_start_point))
    {
        return true;
    }
    if(d4 == 0 && judge_on_segment(right_start_point, right_end_point, left_end_point))
    {
        return true;
    }
    return false;
}
/****************************** function ******************************/

/****************************** element ******************************/
/**
 * @brief 搜索十字路口
 * @param void
 * @return void
 * @note 搜索十字路口，用于判断是否有十字路口
 * @author YuYu
 */
void search_cross_road(void)
{
    int16_t row, column;
    float k, b;
    // 清除十字标志位
    if((judge_crossroad.cross_flag == true || judge_crossroad.cross_left_flag == true || judge_crossroad.cross_right_flag == true)
            && verge.left_first_point.flag == 1 && verge.right_first_point.flag == 1
            && verge.left_first_point.row > (WANT_START_ROW - 20) && verge.right_first_point.row > (WANT_START_ROW - 20)
            && verge.left_first_point.row != WANT_START_ROW && verge.right_first_point.row != WANT_START_ROW)
    {
        judge_crossroad.cross_flag = false;
        judge_crossroad.cross_left_flag = false;
        judge_crossroad.cross_right_flag = false;
    }
    // 正入十字
    if((verge.left_first_point.flag == 1 && verge.right_first_point.flag == 1 // verge.left_lose_line.flag == true && verge.right_lose_line.flag == true
        && verge.left_inflection.row > 48 && verge.right_inflection.row > 48 // 下方L点
        && verge.left_inflection.row < 106 && verge.right_inflection.row < 106
        && verge.left[verge.left_inflection.row].column < (START_MIDDLE_COLUMN + 5) && verge.right[verge.right_inflection.row].column > (START_MIDDLE_COLUMN - 5) // 下方L点
        && abs(verge.left_inflection.row - verge.right_inflection.row) < 30 // 防止弯道误判十字
        && abs(verge.left_inflection.column - verge.right_inflection.column) > 46 // 防止弯道误判十字
        && image_buffer[verge.left_inflection.row - 2][verge.left[verge.left_inflection.row].column] == WHITE && image_buffer[verge.left_inflection.row - 5][verge.left[verge.left_inflection.row].column] == WHITE
        && image_buffer[verge.right_inflection.row - 2][verge.right[verge.right_inflection.row].column] == WHITE && image_buffer[verge.right_inflection.row - 5][verge.right[verge.right_inflection.row].column] == WHITE)
        || judge_crossroad.cross_flag == true)
    {
        judge_crossroad.cross_flag = true;
        // 寻找十字高度
        int16_t left_max_row = (int16_t)limit_value((verge.left_inflection.row - 5), (WANT_START_ROW - CROSS_RANG_ROW), (WANT_END_ROW + CROSS_RANG_ROW));
        int16_t right_max_row = (int16_t)limit_value((verge.right_inflection.row - 5), (WANT_START_ROW - CROSS_RANG_ROW), (WANT_END_ROW + CROSS_RANG_ROW));
        int16_t min_row = (WANT_END_ROW + CROSS_RANG_ROW);
        for(row = left_max_row; row >= min_row; row--)
        {
            if(image_buffer[row][verge.left[verge.left_inflection.row].column] == BLACK)
            {
                judge_crossroad.left_blank_height = verge.left_inflection.row - row;
                break;
            }
            else if(row == min_row)
            {
                judge_crossroad.left_blank_height = verge.left_inflection.row;
            }
        }
        for(row = right_max_row; row >= min_row; row--)
        {
            if(image_buffer[row][verge.right[verge.right_inflection.row].column] == BLACK)
            {
                judge_crossroad.right_blank_height = verge.right_inflection.row - row;
                break;
            }
            else if(row == min_row)
            {
                judge_crossroad.right_blank_height = verge.right_inflection.row;
            }
        }
        // 寻找左右尖点
        int16_t left_shangjiandian_row = 0;
        int16_t left_shangjiandian_column = 0;
        bool left_shangjiandian_flag = false;
        int16_t right_shangjiandian_row = 0;
        int16_t right_shangjiandian_column = 0;
        bool right_shangjiandian_flag = false;
        int16_t find_left_max_row = (int16_t)limit_value((verge.left_inflection.row - judge_crossroad.left_blank_height + SEARCH_HEIGHT), (WANT_START_ROW - CROSS_RANG_ROW), (WANT_END_ROW + CROSS_RANG_ROW));
        int16_t find_right_max_row = (int16_t)limit_value((verge.right_inflection.row - judge_crossroad.right_blank_height + SEARCH_HEIGHT), (WANT_START_ROW - CROSS_RANG_ROW), (WANT_END_ROW + CROSS_RANG_ROW));
        int16_t find_min_row = (WANT_END_ROW + CROSS_RANG_ROW);
        int16_t find_left_max_col = (int16_t)limit_value((verge.right[verge.right_inflection.row].column - 10), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        int16_t find_left_min_col = (int16_t)limit_value((verge.left[verge.left_inflection.row].column - 5), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        int16_t find_right_max_col = (int16_t)limit_value((verge.right[verge.right_inflection.row].column + 5), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        int16_t find_right_min_col = (int16_t)limit_value((verge.left[verge.left_inflection.row].column + 10), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        for(row = find_left_max_row; row >= find_min_row; row--) // 寻找左尖点
        {
            for(column = find_left_min_col; column < find_left_max_col; column++)
            {
                if(image_buffer[row - 2][column - 2] == BLACK && image_buffer[row - 1][column + 3] == WHITE
                        && image_buffer[row][column] == BLACK && image_buffer[row][column + 3] == WHITE
                        && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                        )
                {
                    left_shangjiandian_row = row;
                    left_shangjiandian_column = column;
                    left_shangjiandian_flag = true;
                    break;
                }
            }
            if(left_shangjiandian_flag == true)
            {
                break;
            }
        }
        for(row = find_right_max_row; row >= find_min_row; row--) // 寻找右尖点
        {
            for(column = find_right_max_col; column > find_right_min_col; column--)
            {
                if(image_buffer[row - 1][column - 3] == WHITE && image_buffer[row - 2][column + 2] == BLACK
                        && image_buffer[row][column - 3] == WHITE && image_buffer[row][column] == BLACK
                        && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                        )
                {
                    right_shangjiandian_row = row;
                    right_shangjiandian_column = column;
                    right_shangjiandian_flag = true;
                    break;
                }
            }
            if(right_shangjiandian_flag == true)
            {
                break;
            }
        }
        // 十字补线
        if(left_shangjiandian_flag == true && right_shangjiandian_flag == true) // 左尖点,右尖点都找到
        {
            k = (verge.left[verge.left_inflection.row].column - left_shangjiandian_column) * 1.0f/(verge.left_inflection.row - left_shangjiandian_row);
            b = verge.left[verge.left_inflection.row].column - k * verge.left_inflection.row;
            linear_function_draw_line(image_buffer, (verge.left_inflection.row + 2), (WANT_END_ROW + 8), k, b, 1);
            k = (verge.right[verge.right_inflection.row].column - right_shangjiandian_column) * 1.0f/(verge.right_inflection.row - right_shangjiandian_row);
            b = verge.right[verge.right_inflection.row].column - k * verge.right_inflection.row;
            linear_function_draw_line(image_buffer, (verge.right_inflection.row + 2), (WANT_END_ROW + 8), k, b, 2);
        }
        else if(left_shangjiandian_flag == true && right_shangjiandian_flag == false) // 找到左尖点,没有找到右尖点
        {
            k = (verge.left[verge.left_inflection.row].column - left_shangjiandian_column) * 1.0f/(verge.left_inflection.row - left_shangjiandian_row);
            b = verge.left[verge.left_inflection.row].column - k * verge.left_inflection.row;
            linear_function_draw_line(image_buffer, (verge.left_inflection.row + 2), (WANT_END_ROW + 8), k, b, 1);
            k = (verge.right[verge.right_inflection.row].column - verge.right[verge.right_first_point.row].column) * 1.0f/(verge.right_inflection.row - verge.right_first_point.row);
            b = verge.right[verge.right_inflection.row].column - k * verge.right_inflection.row;
            linear_function_draw_line(image_buffer, (verge.right_inflection.row + 2), (WANT_END_ROW + 8), k, b, 2);
        }
        else if(left_shangjiandian_flag == false && right_shangjiandian_flag == true) // 没有找到左尖点,找到右尖点
        {
            k = (verge.left[verge.left_inflection.row].column - verge.left[verge.left_first_point.row].column) * 1.0f/(verge.left_inflection.row - verge.left_first_point.row);
            b = verge.left[verge.left_inflection.row].column - k * verge.left_inflection.row;
            linear_function_draw_line(image_buffer, (verge.left_inflection.row + 2), (WANT_END_ROW + 8), k, b, 1);
            k = (verge.right[verge.right_inflection.row].column - right_shangjiandian_column) * 1.0f/(verge.right_inflection.row - right_shangjiandian_row);
            b = verge.right[verge.right_inflection.row].column - k * verge.right_inflection.row;
            linear_function_draw_line(image_buffer, (verge.right_inflection.row + 2), (WANT_END_ROW + 8), k, b, 2);
        }
        else // 没有找到左尖点,也没有找到右尖点
        {
            k = (verge.left[verge.left_inflection.row].column - verge.left[verge.left_first_point.row].column) * 1.0f/(verge.left_inflection.row - verge.left_first_point.row);
            b = verge.left[verge.left_inflection.row].column - k * verge.left_inflection.row;
            linear_function_draw_line(image_buffer, (verge.left_inflection.row + 2), (WANT_END_ROW + 8), k, b, 1);
            k = (verge.right[verge.right_inflection.row].column - verge.right[verge.right_first_point.row].column) * 1.0f/(verge.right_inflection.row - verge.right_first_point.row);
            b = verge.right[verge.right_inflection.row].column - k * verge.right_inflection.row;
            linear_function_draw_line(image_buffer, (verge.right_inflection.row + 2), (WANT_END_ROW + 8), k, b, 2);
        }
        // 更新边界终点
        verge.left_last_point.row = WANT_END_ROW + 8;
        verge.left_last_point.column = verge.left[WANT_END_ROW + 8].column;
        verge.left_last_point.column = limit_value(verge.left_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.left_last_point.flag = 1;
        verge.right_last_point.row = WANT_END_ROW + 8;
        verge.right_last_point.column = verge.right[WANT_END_ROW + 8].column;
        verge.right_last_point.column = limit_value(verge.right_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.right_last_point.flag = 1;
    }
    // 左斜入十字
    else if((verge.left_first_point.flag == 1 && (verge.right_first_point.row < 70 || verge.right_first_point.row == WANT_START_ROW)
        && verge.left_inflection.row > 48 && verge.left[verge.left_inflection.row].column < (START_MIDDLE_COLUMN + 20) // 下方L点
        && verge.left[verge.left_inflection.row].column > (LEFT_BOUNDARY_COLUMN + 28)
        && image_buffer[verge.left_inflection.row - 2][verge.left[verge.left_inflection.row].column] == WHITE
        && image_buffer[verge.left_inflection.row - 5][verge.left[verge.left_inflection.row].column] == WHITE
        && ((verge.right[verge.right_last_point.row].column - verge.left[verge.left_inflection.row].column) > 36 || verge.right_last_point.row == WANT_START_ROW))
        || judge_crossroad.cross_left_flag == true)
    {
        judge_crossroad.cross_left_flag = true;
        // 寻找左尖点
        int16_t left_shangjiandian_row = 0;
        int16_t left_shangjiandian_column = 0;
        bool left_shangjiandian_flag = false;
        int16_t find_left_max_row = (int16_t)limit_value((verge.left_inflection.row - CROSS_RANG_ROW), (WANT_START_ROW - CROSS_RANG_ROW), (WANT_END_ROW + CROSS_RANG_ROW));
        int16_t find_min_row = (WANT_END_ROW + CROSS_RANG_ROW);
        int16_t find_left_max_col = (int16_t)limit_value((verge.left[verge.left_inflection.row].column + 40), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        int16_t find_left_min_col = (int16_t)limit_value((verge.left[verge.left_inflection.row].column - CROSS_RANG_COL), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        for(row = find_left_max_row; row >= find_min_row; row--) // 寻找左尖点
        {
            for(column = find_left_max_col; column >= find_left_min_col; column--)
            {
                if(image_buffer[row - 2][column - 2] == BLACK && image_buffer[row - 1][column + 3] == WHITE
                        && image_buffer[row][column] == BLACK && image_buffer[row][column + 3] == WHITE
                        && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                        )
                {
                    left_shangjiandian_row = row;
                    left_shangjiandian_column = column;
                    left_shangjiandian_flag = true;
                    break;
                }
            }
            if(left_shangjiandian_flag == true)
            {
                break;
            }
        }
        if(left_shangjiandian_flag == true) // 找到尖点
        {
            k = (verge.left[verge.left_inflection.row].column - left_shangjiandian_column) * 1.0f/(verge.left_inflection.row - left_shangjiandian_row);
            b = verge.left[verge.left_inflection.row].column - k * verge.left_inflection.row;
            linear_function_draw_line(image_buffer, (verge.left_inflection.row + 2), (WANT_END_ROW + 8), k, b, 1);
        }
        else // 没有找到尖点
        {
            k = (verge.left[verge.left_inflection.row].column - verge.left[verge.left_first_point.row].column) * 1.0f/(verge.left_inflection.row - verge.left_first_point.row);
            b = verge.left[verge.left_inflection.row].column - k * verge.left_inflection.row;
            linear_function_draw_line(image_buffer, (verge.left_inflection.row + 2), (WANT_END_ROW + 8), k, b, 1);
        }
        // 更新边界终点
        verge.left_last_point.row = WANT_END_ROW + 8;
        verge.left_last_point.column = verge.left[WANT_END_ROW + 8].column;
        verge.left_last_point.column = limit_value(verge.left_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.left_last_point.flag = 1;
    }
    // 右斜入十字
    else if((verge.right_first_point.flag == 1 && (verge.left_first_point.row < 70 || verge.left_first_point.row == WANT_START_ROW)
        && verge.right_inflection.row > 48 && verge.right[verge.right_inflection.row].column > (START_MIDDLE_COLUMN - 20) // 下方L点
        && verge.right[verge.right_inflection.row].column < (RIGHT_BOUNDARY_COLUMN - 28)
        && image_buffer[verge.right_inflection.row - 2][verge.right[verge.right_inflection.row].column] == WHITE
        && image_buffer[verge.right_inflection.row - 5][verge.right[verge.right_inflection.row].column] == WHITE
        && ((verge.right[verge.right_inflection.row].column - verge.left[verge.left_last_point.row].column) > 36 || verge.left_last_point.row == WANT_START_ROW))
        || judge_crossroad.cross_right_flag == true)
    {
        judge_crossroad.cross_right_flag = true; // 车头朝右入十字标志
        // 寻找右尖点
        int16_t right_shangjiandian_row = 0;
        int16_t right_shangjiandian_column = 0;
        bool right_shangjiandian_flag = false;
        int16_t find_right_max_row = (int16_t)limit_value((verge.right_inflection.row - CROSS_RANG_ROW), (WANT_START_ROW - CROSS_RANG_ROW), (WANT_END_ROW + CROSS_RANG_ROW));
        int16_t find_min_row = (WANT_END_ROW + CROSS_RANG_ROW);
        int16_t find_right_max_col = (int16_t)limit_value((verge.right[verge.right_inflection.row].column + CROSS_RANG_COL), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        int16_t find_right_min_col = (int16_t)limit_value((verge.right[verge.right_inflection.row].column - 40), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        for(row = find_right_max_row; row >= find_min_row; row--) // 寻找右尖点
        {
            for(column = find_right_min_col; column <= find_right_max_col; column++)
            {
                if(image_buffer[row - 1][column - 3] == WHITE && image_buffer[row - 2][column + 2] == BLACK
                        && image_buffer[row][column - 3] == WHITE && image_buffer[row][column] == BLACK
                        && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                        )
                {
                    right_shangjiandian_row = row;
                    right_shangjiandian_column = column;
                    right_shangjiandian_flag = true;
                    break;
                }
            }
            if(right_shangjiandian_flag == true)
            {
                break;
            }
        }
        if(right_shangjiandian_flag == true) // 找到尖点
        {
            k = (verge.right[verge.right_inflection.row].column - right_shangjiandian_column) * 1.0f/(verge.right_inflection.row - right_shangjiandian_row);
            b = verge.right[verge.right_inflection.row].column - k * verge.right_inflection.row;
            linear_function_draw_line(image_buffer, (verge.right_inflection.row + 2), (WANT_END_ROW + 8), k, b, 2);
        }
        else // 没有找到尖点
        {
            k = (verge.right[verge.right_inflection.row].column - verge.right[verge.right_first_point.row].column) * 1.0f/(verge.right_inflection.row - verge.right_first_point.row);
            b = verge.right[verge.right_inflection.row].column - k * verge.right_inflection.row;
            linear_function_draw_line(image_buffer, (verge.right_inflection.row + 2), (WANT_END_ROW + 8), k, b, 2);
        }
        // 更新边界终点
        verge.right_last_point.row = WANT_END_ROW + 8;
        verge.right_last_point.column = verge.right[WANT_END_ROW + 8].column;
        verge.right_last_point.column = limit_value(verge.right_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.right_last_point.flag = 1;
    }
}

/**
 * @brief 搜索斑马线
 * @param void
 * @return void
 * @note 搜索斑马线，用于判断是否有斑马线
 * @author YuYu
 */
void search_zebra_line(void)
{
    uint8_t i, j;
    uint8_t block_times = 0;
    uint8_t black_blocks = 0;
    uint8_t cursor = 0;
    if(judge_zebra_line.flag == 1)
    {
        for(i = ZEBRA_START_ROW; i >= ZEBRA_END_ROW; i--)
        {
            black_blocks = 0;
            cursor = 0;
            // 左边界到右边界之间寻找黑方块
            for(j = verge.left[i].column + 1; j < verge.right[i].column - 1; j++) 
            {
                if(image_buffer[i][j] == BLACK)
                {
                    cursor++;
                }
                else
                {
                    if(cursor >= 3 && cursor <= 20)
                    {
                        black_blocks++;
                    }
                    else if(cursor > 50)
                    {
                        break;
                    }
                    cursor = 0;
                }
            }
            if(black_blocks >= ZEBRA_MIN_TIMES && black_blocks <= ZEBRA_MAX_TIMES) // 斑马线黑方块个数大于4个才计数
            {
                block_times++;
            }
            else if(black_blocks < ZEBRA_MIN_TIMES)
            {
                break;
            }
        }
        if(block_times >= 2) // 两次判断斑马线防止误判
        {
            block_times = 0;
            judge_zebra_line.flag = 2;
        }
    }
}

/**
 * @brief 检测区域内砖红色像素数量
 * @param min_row 最小行(UVC坐标)
 * @param max_row 最大行(UVC坐标)
 * @param min_col 最小列(UVC坐标)
 * @param max_col 最大列(UVC坐标)
 * @return 砖红色像素个数
 */
static int16_t roadblock_red_count(int16_t min_row, int16_t max_row, int16_t min_col, int16_t max_col)
{
    if(rgb_image == NULL)
    {
        return 0;
    }
    // 边界保护
    min_row = min_row < 0 ? 0 : min_row;
    max_row = max_row > (int16_t)(UVC_HEIGHT - 1) ? (int16_t)(UVC_HEIGHT - 1) : max_row;
    min_col = min_col < 0 ? 0 : min_col;
    max_col = max_col > (int16_t)(UVC_WIDTH - 1) ? (int16_t)(UVC_WIDTH - 1) : max_col;
    if(min_row >= max_row || min_col >= max_col) 
    {
        return 0;
    }
    int16_t red_count = 0;
    for(int16_t r = min_row; r <= max_row; r++)
    {
        for(int16_t c = min_col; c <= max_col; c++)
        {
            uint16_t pixel = rgb_image[r * UVC_WIDTH + c];
            uint8_t R = (pixel >> 11) & 0x1F;  // 5-bit
            uint8_t G = (pixel >> 5)  & 0x3F;  // 6-bit
            uint8_t B =  pixel        & 0x1F;  // 5-bit
            // 砖红色: R明显大于G, G大于B, 且R足够高
            if(R > 18 && R > G && G > B)
            {
                red_count++;
            }
        }
    }
    return red_count;
}

/**
 * @brief 搜索避障
 * @param void
 * @return void
 * @note 搜索避障，用于判断是否有避障
 * @author YuYu
 */
void search_roadblock(void)
{
    if(element_cooldown.roadblock_cooldown > 0)
    {
        element_cooldown.roadblock_cooldown--;
        return; // 冷却中，不检测避障
    }
    static uint8_t road_block_type = 0;
    judge_roadblock.left_line = verge.left_lose_line.row;
    judge_roadblock.right_line = verge.right_lose_line.row;
    if(judge_roadblock.left_roadblock_flag == false && judge_roadblock.right_roadblock_flag == false)
    {
        // 检测起点
        bool first_point_flag = false;
        if(verge.left[verge.left_first_point.row].flag == 1 && verge.right[verge.right_first_point.row].flag == 1
            && verge.left_first_point.row > (WANT_START_ROW - 15) && verge.right_first_point.row > (WANT_START_ROW - 15))
        {
            first_point_flag = true; // 起点
        }
        // 检测丢线
        bool lose_line_flag = false;
        if((judge_roadblock.left_line > OBSTACLE_JUDGE_ROW && verge.left[judge_roadblock.left_line].column < (START_MIDDLE_COLUMN - 10)) // 左避障
            || 
            (judge_roadblock.right_line > OBSTACLE_JUDGE_ROW && verge.right[judge_roadblock.right_line].column > (START_MIDDLE_COLUMN + 10))) // 右避障
        {
            lose_line_flag = true;
        }
        // 检测终点
        bool last_point_flag = false;
        if((verge.left[verge.left_last_point.row].column < (START_MIDDLE_COLUMN - 10) && verge.right[verge.right_last_point.row].column > (START_MIDDLE_COLUMN - 10)) // 左避障
            || 
            (verge.left[verge.left_last_point.row].column < (START_MIDDLE_COLUMN + 10) && verge.right[verge.right_last_point.row].column > (START_MIDDLE_COLUMN + 10))) // 右避障
        {
            last_point_flag = true;
        }
        // printf("first_point_flag = %d, lose_line_flag = %d, last_point_flag = %d\n", first_point_flag, lose_line_flag, last_point_flag);
        if(first_point_flag == true && lose_line_flag == true && last_point_flag == true)
        {
            int16_t lose_line_differ = judge_roadblock.left_line - judge_roadblock.right_line; // 左右丢线行差值
            if(lose_line_differ > OBSTACLE_JUDGE_DIFFER) // 左避障
            {
                bool image_middle_white = image_buffer[judge_roadblock.left_line][(START_MIDDLE_COLUMN + 10)] == WHITE;
                uint8_t col = verge.left[judge_roadblock.left_line].column;
                uint8_t c1 = (col + 6) > RIGHT_BOUNDARY_COLUMN ? RIGHT_BOUNDARY_COLUMN : (col + 6);
                uint8_t c2 = (col + 3) > RIGHT_BOUNDARY_COLUMN ? RIGHT_BOUNDARY_COLUMN : (col + 3);
                bool image_judge = (image_buffer[judge_roadblock.left_line - 6][c1] == BLACK) && (image_buffer[judge_roadblock.left_line - 3][c2] == BLACK) 
                                    && (image_buffer[judge_roadblock.left_line + 3][c2] == WHITE);
                // 检测砖红色像素
                int16_t red_row_start = (judge_roadblock.left_line * 2) - 24;
                int16_t red_row_end   = (judge_roadblock.left_line * 2);
                int16_t red_col_start = (col * 2);
                int16_t red_col_end   = (col * 2) + 30;
                int16_t red_pixels = roadblock_red_count(red_row_start, red_row_end, red_col_start, red_col_end);
                // printf("red_pixels = %d\n", red_pixels);
                bool has_brick_red = (red_pixels > 150); // 20%
                if(image_middle_white == true && image_judge == true && has_brick_red == true
                    && abs(verge.left_inflection.row - judge_roadblock.left_line) <= 2
                    && abs(verge.left[verge.left_inflection.row].column - verge.left[judge_roadblock.left_line].column) <= 5)
                {
                    road_block_type = 1;
                    judge_roadblock.left_roadblock_flag = true;
                    judge_roadblock.right_roadblock_flag = false;
                }
                // printf("image_middle_white = %d, image_judge = %d\n", image_middle_white, image_judge);
            }
            else if(lose_line_differ < -OBSTACLE_JUDGE_DIFFER) // 右避障
            {
                bool image_middle_white = image_buffer[judge_roadblock.right_line][(START_MIDDLE_COLUMN - 10)] == WHITE;
                uint8_t col = verge.right[judge_roadblock.right_line].column;
                uint8_t c1 = (col - 6) < LEFT_BOUNDARY_COLUMN ? LEFT_BOUNDARY_COLUMN : (col - 6);
                uint8_t c2 = (col - 3) < LEFT_BOUNDARY_COLUMN ? LEFT_BOUNDARY_COLUMN : (col - 3);
                bool image_judge = (image_buffer[judge_roadblock.right_line - 6][c1] == BLACK) && (image_buffer[judge_roadblock.right_line - 3][c2] == BLACK)
                                    && (image_buffer[judge_roadblock.right_line + 3][c2] == WHITE);
                // 检测砖红色像素
                int16_t red_row_start = (judge_roadblock.right_line * 2) - 24;
                int16_t red_row_end = (judge_roadblock.right_line * 2);
                int16_t red_col_start = (col * 2) - 30;
                int16_t red_col_end = (col * 2);
                int16_t red_pixels = roadblock_red_count(red_row_start, red_row_end, red_col_start, red_col_end);
                // printf("red_pixels = %d\n", red_pixels);
                bool has_brick_red = (red_pixels > 150); // 20%
                if(image_middle_white == true && image_judge == true && has_brick_red == true
                    && abs(verge.right_inflection.row - judge_roadblock.right_line) <= 2
                    && abs(verge.right[verge.right_inflection.row].column - verge.right[judge_roadblock.right_line].column) <= 5)
                {
                    road_block_type = 1;
                    judge_roadblock.left_roadblock_flag = false;
                    judge_roadblock.right_roadblock_flag = true;
                }
                // printf("image_middle_white = %d, image_judge = %d\n", image_middle_white, image_judge);
            }
            // printf("left_roadblock_flag = %d, right_roadblock_flag = %d\n", judge_roadblock.left_roadblock_flag, judge_roadblock.right_roadblock_flag);
        }
    }
    else if(judge_roadblock.left_roadblock_flag == true)
    {
        static float type2_k = 0.0f;
        static float type2_b = 0.0f;
        if(road_block_type == 1)
        {
            float type1_k = (verge.left[verge.left_first_point.row - 5].column - verge.left[verge.left_last_point.row + 5].column) * 1.0f/((verge.left_first_point.row - 5) - (verge.left_last_point.row + 5));
            float type1_b = (verge.left[judge_roadblock.left_line].column + 10) - type1_k * judge_roadblock.left_line;
            linear_function_draw_line(image_buffer, judge_roadblock.left_line, verge.right_last_point.row, type1_k, type1_b, 1);
            if((verge.left_first_point.row - verge.left_last_point.row) < 25)
            {
                road_block_type = 2;
                type2_k = type1_k;
                type2_b = type1_b;
            }
        }
        else if(road_block_type == 2)
        {
            linear_function_draw_line(image_buffer, WANT_START_ROW, verge.right_last_point.row, type2_k, type2_b, 1);
        }
        // 更新边界终点
        verge.left_last_point.row = verge.right_last_point.row;
        verge.left_last_point.column = verge.left[verge.right_last_point.row].column;
        verge.left_last_point.column = limit_value(verge.left_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.left_last_point.flag = 1;
    }
    else if(judge_roadblock.right_roadblock_flag == true)
    {
        static float type2_k = 0.0f;
        static float type2_b = 0.0f;
        if(road_block_type == 1)
        {
            float type1_k = (verge.right[verge.right_first_point.row - 5].column - verge.right[verge.right_last_point.row + 5].column) * 1.0f/((verge.right_first_point.row - 5) - (verge.right_last_point.row + 5));
            float type1_b = (verge.right[judge_roadblock.right_line].column - 10) - type1_k * judge_roadblock.right_line;
            linear_function_draw_line(image_buffer, judge_roadblock.right_line, verge.left_last_point.row, type1_k, type1_b, 2);
            if((verge.right_first_point.row - verge.right_last_point.row) < 25)
            {
                road_block_type = 2;
                type2_k = type1_k;
                type2_b = type1_b;
            }
        }
        else if(road_block_type == 2)
        {
            linear_function_draw_line(image_buffer, WANT_START_ROW, verge.left_last_point.row, type2_k, type2_b, 2);
        }
        // 更新边界终点
        verge.right_last_point.row = verge.left_last_point.row;
        verge.right_last_point.column = verge.right[verge.left_last_point.row].column;
        verge.right_last_point.column = limit_value(verge.right_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.right_last_point.flag = 1;
    }
    if(road_block_type == 2)
    {
        int16_t left_count = encoder.left;
        int16_t right_count = encoder.right;
        // printf("left_count: %d, right_count: %d\n", left_count, right_count);
        judge_roadblock.encoder_count += (int32_t)(left_count + right_count);
        if(judge_roadblock.encoder_count > OBSTACLE_DISTANCE)
        {
            judge_roadblock.left_roadblock_flag = false;
            judge_roadblock.right_roadblock_flag = false;
            judge_roadblock.encoder_count = 0;
            road_block_type = 0;
            element_cooldown.roadblock_cooldown = 50; // 完成避障后设置冷却时间
        }
    }
}

/**
 * @brief 搜索坡道
 * @param void
 * @return void
 * @note 搜索坡道，用于判断是否有坡道
 * @author YuYu
 */
void search_ramp(void)
{
    if(element_cooldown.ramp_cooldown > 0)
    {
        element_cooldown.ramp_cooldown--;
        return; // 冷却中，不检测坡道
    }
    if(judge_ramp.flag == false) // 进入坡道
    {
        // 检测宽度
        uint8_t judge_times = 0;
        bool ramp_times_condition = false;
        for(uint8_t i = RAMP_JUDGE_START_ROW; i > RAMP_JUDGE_END_ROW; i--)
        {
            if(verge.left[i].flag && verge.right[i].flag)
            {
                int16_t current_width = verge.right[i].column - verge.left[i].column;
                if(current_width - road_differ[i] > RAMP_JUDGE_WIDTH)
                {
                    judge_times++;
                }
            }
        }
        if(judge_times >= RAMP_JUDGE_TIMES)
        {
            ramp_times_condition = true;
        }
        // 计算起点到终点宽度
        bool ramp_width_condition = false;
        judge_ramp.left_width = abs(verge.left[(verge.left_last_point.row + 5)].column - verge.left[(verge.left_first_point.row - 5)].column);
        judge_ramp.right_width = abs(verge.right[(verge.right_last_point.row + 5)].column - verge.right[(verge.right_first_point.row - 5)].column);
        // printf("judge_ramp.left_width = %d, judge_ramp.right_width = %d\n", judge_ramp.left_width, judge_ramp.right_width);
        if(judge_ramp.left_width < RAMP_STANDER_WIDTH && judge_ramp.right_width < RAMP_STANDER_WIDTH)
        {
            ramp_width_condition = true;
        }
        if(ramp_times_condition && ramp_width_condition
            && verge.left_first_point.row > (WANT_START_ROW - 10) && verge.right_first_point.row > (WANT_START_ROW - 10)
            && verge.left_last_point.row < (WANT_END_ROW + 15) && verge.right_last_point.row < (WANT_END_ROW + 15)
            && verge.left[verge.left_last_point.row].column < START_MIDDLE_COLUMN && verge.right[verge.right_last_point.row].column > START_MIDDLE_COLUMN
            )
        {
            judge_ramp.flag = true;
        }
        // printf("judge_times = %d\n", judge_times);
        // printf("ramp_times_condition = %d, ramp_width_condition = %d\n", ramp_times_condition, ramp_width_condition);
        // printf("judge_ramp.flag = %d\n", judge_ramp.flag);
    }
    else if(judge_ramp.flag == true) // 退出坡道
    {
        int16_t left_count = encoder.left;
        int16_t right_count = encoder.right;
        judge_ramp.encoder_count += (int32_t)(left_count + right_count);
        if(judge_ramp.encoder_count > RAMP_DISTANCE)
        {
            judge_ramp.flag = false;
            judge_ramp.encoder_count = 0;
            element_cooldown.ramp_cooldown = 100; // 完成坡道后设置冷却时间
        }
        // printf("judge_ramp.encoder_count = %d\n", judge_ramp.encoder_count);
    }
}

/**
 * @brief 搜索左环岛
 * @param void
 * @return void
 * @note 搜索左环岛，用于判断是否有左环岛
 * @author YuYu
 */
void search_left_island(void)
{
    if(element_cooldown.island_cooldown > 0)
    {
        element_cooldown.island_cooldown--;
        return; // 冷却中，不检测左环岛
    }
    float k, b;
    uint8_t row, column;
    // 判断左环岛
    if(judge_island.left_island_type == 0)
    {
        bool first_point_flag = false;
        if(verge.left_first_point.flag == 1 && verge.right_first_point.flag == 1 
            && verge.left_first_point.row > (WANT_START_ROW - 20) && verge.right_first_point.row > (WANT_START_ROW - 20))
        {
            first_point_flag = true; // 起点
        }
        bool lose_line_flag = false;
        if(verge.left_lose_line.flag == 1 && verge.right_lose_line.flag == 0 && verge.left_lose_line.row > JUDGE_ISLAND_MIN_ROW && verge.right_lose_line.row < JUDGE_ISLAND_MAX_ROW)
        {
            lose_line_flag = true; // 丢线
        }
        bool last_point_flag = false;
        if(verge.left_last_point.row > JUDGE_ISLAND_MIN_ROW && verge.right_last_point.row < JUDGE_ISLAND_MAX_ROW
            && verge.left[verge.left_last_point.row].column < (START_MIDDLE_COLUMN + 10) && verge.right[verge.right_last_point.row].column > (START_MIDDLE_COLUMN - 10))
        {
            last_point_flag = true; // 终点
        }
        bool inflection_flag = false;
        if(verge.left_inflection.row > JUDGE_ISLAND_MIN_ROW && verge.right_inflection.row < JUDGE_ISLAND_MAX_ROW
            && image_buffer[(verge.left_inflection.row - 2)][(verge.left[verge.left_inflection.row].column - 8) < LEFT_BOUNDARY_COLUMN ? LEFT_BOUNDARY_COLUMN : (verge.left[verge.left_inflection.row].column - 8)] == WHITE
            && image_buffer[(verge.left_inflection.row - 5)][(verge.left[verge.left_inflection.row].column - 8) < LEFT_BOUNDARY_COLUMN ? LEFT_BOUNDARY_COLUMN : (verge.left[verge.left_inflection.row].column - 8)] == WHITE
            && image_buffer[(verge.left_inflection.row - 8)][(verge.left[verge.left_inflection.row].column - 8) < LEFT_BOUNDARY_COLUMN ? LEFT_BOUNDARY_COLUMN : (verge.left[verge.left_inflection.row].column - 8)] == WHITE)
        {
            inflection_flag = true; // 拐点
        }
        if(first_point_flag == true && last_point_flag == true && lose_line_flag == true && inflection_flag == true
            && abs(verge.right_last_point.row - verge.right_inflection.row) <= 2
            && abs(verge.right[verge.right_last_point.row].column - verge.right[verge.right_inflection.row].column) <= 5)
        {
            uint8_t judge_island_column = (verge.left[verge.left_last_point.row].column - 8) < LEFT_BOUNDARY_COLUMN ? LEFT_BOUNDARY_COLUMN: (verge.left[verge.left_last_point.row].column - 8);
            uint8_t white_count = search_black_white_transition(image_buffer, verge.left_last_point.row, WANT_END_ROW, judge_island_column, 1); // 白色像素点数量
            uint8_t black_count = search_black_white_transition(image_buffer, verge.left_last_point.row, WANT_END_ROW, judge_island_column, 2); // 黑色像素点数量
            if(white_count > JUDGE_ISLAND_WHITE_COUNT && black_count > JUDGE_ISLAND_BLACK_COUNT)
            {
                judge_island.left_island_type = 1;
            }
        }
    }
    // 左环岛第一个缺口补线
    if(judge_island.left_island_type == 1)
    {
        if(verge.left_first_point.row <= (WANT_START_ROW - 10))
        {
            k = -(verge.right[verge.right_first_point.row - 10].column - verge.right[verge.right_last_point.row + 10].column) * 1.0f/((verge.right_first_point.row - 10) - (verge.right_last_point.row + 10));
            b = (LEFT_BOUNDARY_COLUMN + 6) - k * (WANT_START_ROW - 10);
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), k, b, 1);
            if(judge_island.left_type1_flag == false)
            {
                judge_island.left_type1_flag = true;
            }
        }
        else if(verge.left_first_point.row > (WANT_START_ROW - 10) && judge_island.left_type1_flag == true)
        {
            k = -(verge.right[verge.right_first_point.row - 10].column - verge.right[verge.right_last_point.row + 10].column) * 1.0f/((verge.right_first_point.row - 10) - (verge.right_last_point.row + 10));
            b = (LEFT_BOUNDARY_COLUMN + 12) - k * (WANT_START_ROW - 10);
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), k, b, 1);
        }
        else
        {
            k = (verge.left[verge.left_inflection.row].column - verge.left[verge.left_first_point.row].column) * 1.0f/(verge.left_inflection.row - verge.left_first_point.row);
            b = verge.left[verge.left_inflection.row].column - k * verge.left_inflection.row;
            linear_function_draw_line(image_buffer, (verge.left_inflection.row + 1), (WANT_END_ROW + 8), k, b, 1);
        }
        // 更新边界终点
        verge.left_last_point.row = (WANT_END_ROW + 8);
        verge.left_last_point.column = verge.left[WANT_END_ROW + 8].column;
        verge.left_last_point.column = limit_value(verge.left_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.left_last_point.flag = 1;
        // 退出环岛一阶段
        if(judge_island.left_type1_flag == true && verge.left_first_point.row >= (WANT_START_ROW - 10)
            && abs(verge.left[verge.left_inflection.row].column - verge.left[verge.left_first_point.row].column) <= 15
            && verge.left_inflection.row > (WANT_START_ROW - 20))
        {
            judge_island.left_island_type = 2;
            judge_island.left_type1_flag = false;
        }
    }
    // 进入环岛
    if(judge_island.left_island_type == 2)
    {
        static uint8_t lefth2_shangjiandian_row = 0;
        static uint8_t lefth2_shangjiandian_column = 0;
        static bool lefth2_shangjiandian_flag = false;
        // 寻找尖点
        if(lefth2_shangjiandian_flag == false)
        {
            int16_t max_row = limit_value((verge.right_last_point.row + (3 * judge_island.range_row)), WANT_START_ROW, verge.right_last_point.row);
            int16_t min_row = verge.right_last_point.row;
            int16_t min_column = limit_value((verge.right[verge.right_last_point.row].column - 4 * judge_island.range_column), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            int16_t max_column = limit_value((verge.right[verge.right_last_point.row].column - 5), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            bool found_point = false;
            for(row = max_row; row > min_row; row--)
            {
                for(column = max_column; column > min_column; column--)
                {
                    if(image_buffer[row - 1][column + 3] == WHITE && image_buffer[row - 1][column] == BLACK
                            && image_buffer[row][column + 3] == WHITE && image_buffer[row][column] == BLACK
                            && image_buffer[row + 1][column + 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column - 3] == WHITE
                              )
                    {
                        lefth2_shangjiandian_row = row;
                        lefth2_shangjiandian_column = column;
                        lefth2_shangjiandian_flag = true;
                        found_point = true;
                        break;
                    }
                }
                if(found_point == true)
                {
                    break;
                }
            }
            if(!found_point)
            {
                lefth2_shangjiandian_flag = false;
            }
        }
        else
        {
            int16_t max_row = limit_value((lefth2_shangjiandian_row + judge_island.range_row), WANT_START_ROW, lefth2_shangjiandian_row);
            int16_t min_row = limit_value((lefth2_shangjiandian_row - judge_island.range_row), lefth2_shangjiandian_row, WANT_END_ROW);
            int16_t max_column = limit_value((lefth2_shangjiandian_column + judge_island.range_column), RIGHT_BOUNDARY_COLUMN, lefth2_shangjiandian_column);
            int16_t min_column = limit_value((lefth2_shangjiandian_column - judge_island.range_column), lefth2_shangjiandian_column, LEFT_BOUNDARY_COLUMN);
            bool found_point = false;
            for(row = max_row; row > min_row; row--)
            {
                for(column = max_column; column > min_column; column--)
                {
                    if(image_buffer[row - 1][column + 3] == WHITE && image_buffer[row - 1][column] == BLACK
                            && image_buffer[row][column + 3] == WHITE && image_buffer[row][column] == BLACK
                            && image_buffer[row + 1][column + 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column - 3] == WHITE
                            )
                    {
                        lefth2_shangjiandian_row = row;
                        lefth2_shangjiandian_column = column;
                        lefth2_shangjiandian_flag = true;
                        found_point = true;
                        break;
                    }
                    else
                    {
                        lefth2_shangjiandian_flag = false;
                    }
                }
                if(found_point == true)
                {
                    break;
                }
            }
            if(!found_point)
            {
                lefth2_shangjiandian_flag = false;
            }
        }
        if(lefth2_shangjiandian_flag == true)
        {
            k = (RIGHT_BOUNDARY_COLUMN - (lefth2_shangjiandian_column + 5)) * 1.0f/(WANT_START_ROW - lefth2_shangjiandian_row);
            b = RIGHT_BOUNDARY_COLUMN - k * WANT_START_ROW;
            if(fabs(k) < 1.36)
            {
                judge_island.left_type2_k = 1.36;
                judge_island.left_type2_b = RIGHT_BOUNDARY_COLUMN - judge_island.left_type2_k * WANT_START_ROW;
            }
            else 
            {
                judge_island.left_type2_k = k;
                judge_island.left_type2_b = b;
            }
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), k, b, 2);
        }
        else
        {
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), judge_island.left_type2_k, judge_island.left_type2_b, 2);
        }
        // 更新边界终点
        verge.right_last_point.row = (WANT_END_ROW + 8);
        verge.right_last_point.column = verge.right[(WANT_END_ROW + 8)].column;
        verge.right_last_point.column = limit_value(verge.right_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.right_last_point.flag = 1;
        if(verge.left_inflection.row < (WANT_END_ROW + 30)) // 防止过早如环
        {
            for(uint8_t lrow = WANT_START_ROW; lrow >= WANT_END_ROW; lrow--) // 清除左边界, 切换为单边扫线入环
            {
                verge.left[lrow].column = LEFT_BOUNDARY_COLUMN;
                verge.left[lrow].flag = 0;
            }
        }
        if((lefth2_shangjiandian_row > (WANT_START_ROW - 20) && lefth2_shangjiandian_column > (START_MIDDLE_COLUMN + 40) // (90, 120)
            && verge.right_first_point.row > (WANT_START_ROW - 25) && fabsf(gyro_element.island) > 40.0f) // 280.0f
            || (verge.right_first_point.row > (WANT_START_ROW - 10) && fabsf(gyro_element.island) > 45.0f)) // 320.0f
        {
            lefth2_shangjiandian_row = 0;
            lefth2_shangjiandian_column = 0;
            lefth2_shangjiandian_flag = false;
            judge_island.left_type2_k = 0.0;
            judge_island.left_type2_b = 0.0;
            judge_island.left_island_type = 3;
        }
    }
    // 环岛中
    if(judge_island.left_island_type == 3)
    {
        for(uint8_t lrow = WANT_START_ROW; lrow >= WANT_END_ROW; lrow--) // 清除左边界, 切换为单边扫线
        {
            verge.left[lrow].column = LEFT_BOUNDARY_COLUMN;
            verge.left[lrow].flag = 0;
        }
        if((verge.right_inflection.row > (WANT_END_ROW + 20) && verge.right[verge.right_inflection.row].column > (START_MIDDLE_COLUMN - 20)
            && image_buffer[verge.right_inflection.row - 2][verge.right[verge.right_inflection.row].column] == WHITE
            && image_buffer[verge.right_inflection.row - 5][verge.right[verge.right_inflection.row].column] == WHITE
            && fabsf(gyro_element.island) > 220.0f) // 1700.0f
            // || fabsf(gyro_element.island) > 2200.0f
            )
        {
            judge_island.left_island_type = 4;
        }
    }
    // 出环岛
    if(judge_island.left_island_type == 4)
    {
        static int16_t max_row = 0;
        static int16_t min_row = 0;
        if(judge_island.out_island_flag == false)
        {
            k = ((verge.right_inflection.column - 6) - RIGHT_BOUNDARY_COLUMN) * 1.0f/(verge.right_inflection.row - WANT_START_ROW);
            b = verge.right_inflection.column - k * verge.right_inflection.row;
            judge_island.left_type4_k = k;
            judge_island.left_type4_b = b; 
            max_row = (verge.right_inflection.row + judge_island.range_row) > WANT_START_ROW ? WANT_START_ROW : (verge.right_inflection.row + judge_island.range_row);
            min_row = (WANT_END_ROW + 8);
            linear_function_draw_line(image_buffer, max_row, min_row, k, b, 2);
            if(abs(verge.right_first_point.row - verge.right_inflection.row) < 25)
            {
                judge_island.out_island_flag = true;
            }
        }
        else
        {
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), judge_island.left_type4_k, judge_island.left_type4_b, 2);
        }
        if(judge_island.out_island_flag == true) // 寻找尖点
        {
            static uint8_t lefth4_shangjiandian_row = 0;
            static uint8_t lefth4_shangjiandian_column = 0;
            static bool lefth4_shangjiandian_flag = false;
            // 寻找尖点
            if(lefth4_shangjiandian_flag == false)
            {
                int16_t max_row = limit_value((WANT_START_ROW - 50), WANT_START_ROW, (WANT_END_ROW + 10));
                int16_t min_row = (WANT_END_ROW + 10);
                int16_t min_column = limit_value((verge.right[verge.right_last_point.row].column - 4 * judge_island.range_column), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                int16_t max_column = limit_value((verge.right[verge.right_last_point.row].column - 5), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                bool found_point = false;
                for(row = max_row; row > min_row; row--)
                {
                    for(column = max_column; column > min_column; column--)
                    {
                        if(image_buffer[row - 1][column + 3] == WHITE && image_buffer[row - 1][column] == BLACK
                                && image_buffer[row][column + 3] == WHITE && image_buffer[row][column] == BLACK
                                && image_buffer[row + 1][column + 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column - 3] == WHITE
                                )
                        {
                            lefth4_shangjiandian_row = row;
                            lefth4_shangjiandian_column = column;
                            lefth4_shangjiandian_flag = true;
                            found_point = true;
                            break;
                        }
                    }
                    if(found_point == true)
                    {
                        break;
                    }
                }
                if(!found_point)
                {
                    lefth4_shangjiandian_flag = false;
                }
            }
            else
            {
                int16_t max_row = limit_value((lefth4_shangjiandian_row + judge_island.range_row), WANT_START_ROW, lefth4_shangjiandian_row);
                int16_t min_row = limit_value((lefth4_shangjiandian_row - judge_island.range_row), lefth4_shangjiandian_row, WANT_END_ROW);
                int16_t max_column = limit_value((lefth4_shangjiandian_column + judge_island.range_column), RIGHT_BOUNDARY_COLUMN, lefth4_shangjiandian_column);
                int16_t min_column = limit_value((lefth4_shangjiandian_column - judge_island.range_column), lefth4_shangjiandian_column, LEFT_BOUNDARY_COLUMN);
                bool found_point = false;
                for(row = max_row; row > min_row; row--)
                {
                    for(column = max_column; column > min_column; column--)
                    {
                        if(image_buffer[row - 1][column + 3] == WHITE && image_buffer[row - 1][column] == BLACK
                                && image_buffer[row][column + 3] == WHITE && image_buffer[row][column] == BLACK
                                && image_buffer[row + 1][column + 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column - 3] == WHITE)
                        {
                            lefth4_shangjiandian_row = row;
                            lefth4_shangjiandian_column = column;
                            lefth4_shangjiandian_flag = true;
                            found_point = true;
                            break;
                        }
                        else
                        {
                            lefth4_shangjiandian_flag = false;
                        }
                    }
                    if(found_point == true)
                    {
                        break;
                    }
                }
                if(!found_point)
                {
                    lefth4_shangjiandian_flag = false;
                }
            }
            if(judge_island.out_island_flag == true && verge.right_first_point.row > (WANT_START_ROW - 10)
                && lefth4_shangjiandian_row > (WANT_END_ROW + 18) && lefth4_shangjiandian_column > (LEFT_BOUNDARY_COLUMN + 16) // (48, 18)
                && lefth4_shangjiandian_column < (START_MIDDLE_COLUMN - 20))
            {
                judge_island.out_island_flag = false;
                max_row = 0;
                min_row = 0;
                lefth4_shangjiandian_row = 0;
                lefth4_shangjiandian_column = 0;
                lefth4_shangjiandian_flag = false;
                judge_island.left_type4_k = 0.0;
                judge_island.left_type4_b = 0.0;
                judge_island.left_island_type = 5;
            }
        }
        // 更新边界终点
        verge.right_last_point.row = (WANT_END_ROW + 8);
        verge.right_last_point.column = verge.right[WANT_END_ROW + 8].column;
        verge.right_last_point.column = limit_value(verge.right_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.right_last_point.flag = 1;
        for(uint8_t lrow = WANT_START_ROW; lrow >= WANT_END_ROW; lrow--) // 清除左边界, 切换为单边扫线
        {
            verge.left[lrow].column = LEFT_BOUNDARY_COLUMN;
            verge.left[lrow].flag = 0;
        }
        if(judge_island.out_island_flag == true && verge.right_inflection.row < (WANT_END_ROW + 20) 
            && verge.right_inflection.column > (LEFT_BOUNDARY_COLUMN + 8) && verge.right_inflection.column < (START_MIDDLE_COLUMN - 30)
            && abs(verge.left_inflection.column - verge.right_inflection.column) > 8)
        {
            judge_island.out_island_flag = false;
            max_row = 0;
            min_row = 0;
            judge_island.left_type4_k = 0.0;
            judge_island.left_type4_b = 0.0;
            judge_island.left_island_type = 5;
        }
    }
    // 防止再次进入环岛
    if(judge_island.left_island_type == 5)
    {
        static uint8_t lefth5_shangjiandian_row = 0;
        static uint8_t lefth5_shangjiandian_column = 0;
        static bool lefth5_shangjiandian_flag = false;
        // 寻找尖点
        if(lefth5_shangjiandian_flag == false)
        {
            int16_t max_row = limit_value((verge.right_last_point.row + (3 * judge_island.range_row)), WANT_START_ROW, verge.right_last_point.row);
            int16_t min_row = verge.right_last_point.row;
            int16_t max_column = limit_value((verge.right[verge.right_last_point.row].column - 5), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            int16_t min_column = limit_value((verge.right[verge.right_last_point.row].column - 4 * judge_island.range_column), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            bool found_point = false;
            for(row = max_row; row > min_row; row--)
            {
                for(column = max_column; column > min_column; column--)
                {
                    if(image_buffer[row - 1][column + 3] == WHITE && image_buffer[row - 1][column] == BLACK
                            && image_buffer[row][column + 3] == WHITE && image_buffer[row][column] == BLACK
                            && image_buffer[row + 1][column + 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column - 3] == WHITE
                              )
                    {
                        lefth5_shangjiandian_row = row;
                        lefth5_shangjiandian_column = column;
                        lefth5_shangjiandian_flag = true;
                        found_point = true;
                        break;
                    }
                }
                if(found_point == true)
                {
                    break;
                }
            }
            if(!found_point)
            {
                lefth5_shangjiandian_flag = false;
            }
        }
        else
        {
            int16_t max_row = limit_value((lefth5_shangjiandian_row + judge_island.range_row), WANT_START_ROW, lefth5_shangjiandian_row);
            int16_t min_row = limit_value((lefth5_shangjiandian_row - judge_island.range_row), lefth5_shangjiandian_row, WANT_END_ROW);
            int16_t max_column = limit_value((lefth5_shangjiandian_column + judge_island.range_column), RIGHT_BOUNDARY_COLUMN, lefth5_shangjiandian_column);
            int16_t min_column = limit_value((lefth5_shangjiandian_column - judge_island.range_column), lefth5_shangjiandian_column, LEFT_BOUNDARY_COLUMN);
            bool found_point = false;
            for(row = max_row; row > min_row; row--)
            {
                for(column = max_column; column > min_column; column--)
                {
                    if(image_buffer[row - 1][column + 3] == WHITE && image_buffer[row - 1][column] == BLACK
                            && image_buffer[row][column + 3] == WHITE && image_buffer[row][column] == BLACK
                            && image_buffer[row + 1][column + 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column - 3] == WHITE
                        )
                    {
                        lefth5_shangjiandian_row = row;
                        lefth5_shangjiandian_column = column;
                        lefth5_shangjiandian_flag = true;
                        found_point = true;
                        break;
                    }
                    else
                    {
                        lefth5_shangjiandian_flag = false;
                    }
                }
                if(found_point == true)
                {
                    break;
                }
            }
            if(!found_point)
            {
                lefth5_shangjiandian_flag = false;
            }
        }
        if(lefth5_shangjiandian_flag == true)
        {
            k = (LEFT_BOUNDARY_COLUMN - (lefth5_shangjiandian_column - 0)) * 1.0f/(WANT_START_ROW - (lefth5_shangjiandian_row));
            b = LEFT_BOUNDARY_COLUMN - k * WANT_START_ROW;
            linear_function_draw_line(image_buffer, WANT_START_ROW, (lefth5_shangjiandian_row - 1), k, b, 1);
            // 更新边界终点
            verge.left_last_point.row = lefth5_shangjiandian_row - 1;
            verge.left_last_point.column = verge.left[(lefth5_shangjiandian_row - 1)].column;
            verge.left_last_point.column = limit_value(verge.left_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            verge.left_last_point.flag = 1;
        }
        if(verge.left_first_point.flag == 1 && verge.right_first_point.flag == 1
            && verge.left_first_point.row > (WANT_START_ROW - 20) && verge.right_first_point.row > (WANT_START_ROW - 20)
            && verge.left_lose_line.row < (WANT_END_ROW + 20) && verge.right_lose_line.row < (WANT_END_ROW + 20))
        {
            gyro_element.island = 0;
            lefth5_shangjiandian_row = 0;
            lefth5_shangjiandian_column = 0;
            lefth5_shangjiandian_flag = false;
            judge_island.left_island_type = 0;
            element_cooldown.island_cooldown = 50; // 完成左环岛后设置冷却时间
        }
    }
}

/**
 * @brief 搜索右环岛
 * @param void
 * @return void
 * @note 搜索右环岛，用于判断是否有右环岛
 * @author YuYu
 */
void search_right_island(void)
{
    if(element_cooldown.island_cooldown > 0)
    {
        element_cooldown.island_cooldown--;
        return; // 冷却中，不检测右环岛
    }
    float k, b;
    uint8_t row, column;
    // 判断右环岛
    if(judge_island.right_island_type == 0)
    {
        bool first_point_flag = false;
        if(verge.left_first_point.flag == 1 && verge.right_first_point.flag == 1 
            && verge.left_first_point.row > (WANT_START_ROW - 20) && verge.right_first_point.row > (WANT_START_ROW - 20))
        {
            first_point_flag = true; // 起点
        }
        bool lose_line_flag = false;
        if(verge.left_lose_line.flag == 0 && verge.right_lose_line.flag == 1 && verge.left_lose_line.row < JUDGE_ISLAND_MAX_ROW && verge.right_lose_line.row > JUDGE_ISLAND_MIN_ROW)
        {
            lose_line_flag = true; // 丢线行
        }
        bool last_point_flag = false;
        if(verge.left_last_point.row < JUDGE_ISLAND_MAX_ROW && verge.right_last_point.row > JUDGE_ISLAND_MIN_ROW
            && verge.left[verge.left_last_point.row].column < (START_MIDDLE_COLUMN + 10) && verge.right[verge.right_last_point.row].column > (START_MIDDLE_COLUMN - 10))
        {
            last_point_flag = true; /// 终点
        }
        bool inflection_flag = false;
        if(verge.left_inflection.row < JUDGE_ISLAND_MAX_ROW && verge.right_inflection.row > JUDGE_ISLAND_MIN_ROW
            && image_buffer[(verge.right_inflection.row - 2)][(verge.right[verge.right_inflection.row].column + 8) > RIGHT_BOUNDARY_COLUMN ?  RIGHT_BOUNDARY_COLUMN: (verge.right[verge.right_inflection.row].column + 8)] == WHITE
            && image_buffer[(verge.right_inflection.row - 5)][(verge.right[verge.right_inflection.row].column + 8) > RIGHT_BOUNDARY_COLUMN ?  RIGHT_BOUNDARY_COLUMN: (verge.right[verge.right_inflection.row].column + 8)] == WHITE
            && image_buffer[(verge.right_inflection.row - 8)][(verge.right[verge.right_inflection.row].column + 8) > RIGHT_BOUNDARY_COLUMN ?  RIGHT_BOUNDARY_COLUMN: (verge.right[verge.right_inflection.row].column + 8)] == WHITE)
        {
            inflection_flag = true;
        }
        if(first_point_flag == true && lose_line_flag == true && last_point_flag == true && inflection_flag == true
            && abs(verge.left_last_point.row - verge.left_inflection.row) <= 2
            && abs(verge.left[verge.left_last_point.row].column - verge.left[verge.left_inflection.row].column) <= 5)
        {
            uint8_t judge_island_column = (verge.right[verge.right_last_point.row].column + 8) > RIGHT_BOUNDARY_COLUMN ? RIGHT_BOUNDARY_COLUMN: (verge.right[verge.right_last_point.row].column + 8);
            uint8_t white_count = search_black_white_transition(image_buffer, verge.right_last_point.row, WANT_END_ROW, judge_island_column, 1);
            uint8_t black_count = search_black_white_transition(image_buffer, verge.right_last_point.row, WANT_END_ROW, judge_island_column, 2);
            if(white_count > JUDGE_ISLAND_WHITE_COUNT && black_count > JUDGE_ISLAND_BLACK_COUNT)
            {
                judge_island.right_island_type = 1;
            }
        }
    }
    // 右环岛第一个缺口补线
    if(judge_island.right_island_type == 1)
    {
        if(verge.right_first_point.row < (WANT_START_ROW - 10))
        {
            k = -(verge.left[(verge.left_first_point.row - 10)].column - verge.left[(verge.left_last_point.row + 10)].column) * 1.0f/((verge.left_first_point.row - 10) - (verge.left_last_point.row + 10));
            b = (RIGHT_BOUNDARY_COLUMN - 6) - k * (WANT_START_ROW - 10);
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), k, b, 2);
            if(judge_island.right_type1_flag == false)
            {
                judge_island.right_type1_flag = true;
            }
        }
        else if(verge.right_first_point.row > (WANT_START_ROW - 10) && judge_island.right_type1_flag == true)
        {
            k = -(verge.left[(verge.left_first_point.row - 10)].column - verge.left[(verge.left_last_point.row + 10)].column) * 1.0f/((verge.left_first_point.row - 10) - (verge.left_last_point.row + 10));
            b = (RIGHT_BOUNDARY_COLUMN - 12) - k * (WANT_START_ROW - 10);
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), k, b, 2);
        }
        else
        {
            k = (verge.right[verge.right_inflection.row].column - verge.right[verge.right_first_point.row].column) * 1.0f/(verge.right_inflection.row - verge.right_first_point.row);
            b = verge.right[verge.right_inflection.row].column - k * verge.right_inflection.row;
            linear_function_draw_line(image_buffer, (verge.right_inflection.row + 1), (WANT_END_ROW + 8), k, b, 2);
        }
        // 更新边界终点
        verge.right_last_point.row = (WANT_END_ROW + 8);
        verge.right_last_point.column = verge.right[(WANT_END_ROW + 8)].column;
        verge.right_last_point.column = limit_value(verge.right_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.right_last_point.flag = 1;
        // 退出环岛一阶段
        if(judge_island.right_type1_flag == true && verge.right_first_point.row >= (WANT_START_ROW - 10)
            && abs(verge.right[verge.right_inflection.row].column - verge.right[verge.right_first_point.row].column) <= 15
            && verge.right_inflection.row > (WANT_START_ROW - 20))
        {
            judge_island.right_island_type = 2;
            judge_island.right_type1_flag = false;
        }
    }
    // 进入环岛
    if(judge_island.right_island_type == 2)
    {
        static uint8_t righth2_shangjiandian_row = 0;
        static uint8_t righth2_shangjiandian_column = 0;
        static bool righth2_shangjiandian_flag = false;
        // 寻找尖点
        if(righth2_shangjiandian_flag == false)
        {
            int16_t max_row = limit_value((verge.left_last_point.row + (3 * judge_island.range_row)), WANT_START_ROW, verge.left_last_point.row);
            int16_t min_row = verge.left_last_point.row;
            int16_t max_column = limit_value((verge.left[verge.left_last_point.row].column + 4 * judge_island.range_column), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            int16_t min_column = limit_value((verge.left[verge.left_last_point.row].column + 5), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            bool found_point = false;
            for(row = max_row; row > min_row; row--)
            {
                for(column = min_column; column < max_column; column++)
                {
                    if(image_buffer[row - 1][column - 3] == WHITE && image_buffer[row - 1][column] == BLACK
                            && image_buffer[row][column - 3] == WHITE && image_buffer[row][column] == BLACK
                            && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                        )
                    {
                        righth2_shangjiandian_row = row;
                        righth2_shangjiandian_column = column;
                        righth2_shangjiandian_flag = true;
                        found_point = true;
                        break;
                    }
                }
                if(found_point)
                {
                    break;
                }
            }
            if(!found_point)
            {
                righth2_shangjiandian_flag = false;
            }
        }
        else
        {
            int16_t max_row = limit_value((righth2_shangjiandian_row + judge_island.range_row), WANT_START_ROW, righth2_shangjiandian_row);
            int16_t min_row = limit_value((righth2_shangjiandian_row - judge_island.range_row), righth2_shangjiandian_row, WANT_END_ROW);
            int16_t max_column = limit_value((righth2_shangjiandian_column + judge_island.range_column), RIGHT_BOUNDARY_COLUMN, righth2_shangjiandian_column);
            int16_t min_column = limit_value((righth2_shangjiandian_column - judge_island.range_column), righth2_shangjiandian_column, LEFT_BOUNDARY_COLUMN);
            bool found_point = false;
            for(row = max_row; row > min_row; row--)
            {
                for(column = min_column; column < max_column; column++)
                {
                    if(image_buffer[row - 1][column - 3] == WHITE && image_buffer[row - 1][column] == BLACK
                            && image_buffer[row][column - 3] == WHITE && image_buffer[row][column] == BLACK
                            && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                            )
                    {
                        righth2_shangjiandian_row = row;
                        righth2_shangjiandian_column = column;
                        righth2_shangjiandian_flag = true;
                        found_point = true;
                        break;
                    }
                }
                if(found_point)
                {
                    break;
                }
            }
            if(!found_point)
            {
                righth2_shangjiandian_flag = false;
            }
        }
        if(righth2_shangjiandian_flag == true)
        {
            k = (LEFT_BOUNDARY_COLUMN - (righth2_shangjiandian_column - 5)) * 1.0f/(WANT_START_ROW - (righth2_shangjiandian_row));
            b = LEFT_BOUNDARY_COLUMN - k * WANT_START_ROW;
            if(abs(k) < 1.36)
            {
                judge_island.right_type2_k = -1.36;
                judge_island.right_type2_b = LEFT_BOUNDARY_COLUMN - judge_island.right_type2_k * WANT_START_ROW;
            }
            else
            {
                judge_island.right_type2_k = k;
                judge_island.right_type2_b = b;
            }
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), k, b, 1);
        }
        else
        {
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), judge_island.right_type2_k, judge_island.right_type2_b, 1);
        }
        // 更新边界终点
        verge.left_last_point.row = (WANT_END_ROW + 8);
        verge.left_last_point.column = verge.left[(WANT_END_ROW + 8)].column;
        verge.left_last_point.column = limit_value(verge.left_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.left_last_point.flag = 1;
        if(verge.left_inflection.row < (WANT_END_ROW + 30))
        {
            for(uint8_t rrow = WANT_START_ROW; rrow >= WANT_END_ROW; rrow--) // 清除右边界, 切换为单边扫线入环
            {
                verge.right[rrow].column = RIGHT_BOUNDARY_COLUMN;
                verge.right[rrow].flag = 0;
            }
        }
        // 退出环岛二阶段
        if((righth2_shangjiandian_row > (WANT_START_ROW - 20) && righth2_shangjiandian_column < (START_MIDDLE_COLUMN - 40) // (90, 40)
            && verge.left_first_point.row > (WANT_START_ROW - 25) && fabsf(gyro_element.island) > 40.0f) // 280.0f
            || (verge.left_first_point.row > (WANT_START_ROW - 10) && fabsf(gyro_element.island) > 45.0f)) // 320.0f
        {
            righth2_shangjiandian_row = 0;
            righth2_shangjiandian_column = 0;
            righth2_shangjiandian_flag = false;
            judge_island.right_type2_k = 0.0;
            judge_island.right_type2_b = 0.0;
            judge_island.right_island_type = 3;
        }
    }
    // 环岛中
    if(judge_island.right_island_type == 3)
    {
        for(uint8_t rrow = WANT_START_ROW; rrow >= WANT_END_ROW; rrow--) // 清除右边界, 切换为单边扫线
        {
            verge.right[rrow].column = RIGHT_BOUNDARY_COLUMN;
            verge.right[rrow].flag = 0;
        }
        if((verge.left_inflection.row > (WANT_END_ROW + 20) && verge.left[verge.left_inflection.row].column < (START_MIDDLE_COLUMN + 20)
            && image_buffer[verge.left_inflection.row - 2][verge.left[verge.left_inflection.row].column] == WHITE
            && image_buffer[verge.left_inflection.row - 5][verge.left[verge.left_inflection.row].column] == WHITE
            && fabsf(gyro_element.island) > 220.0) // 1700.0f
            // || fabsf(gyro_element.island) > 2200.0f
        )
        {
            judge_island.right_island_type = 4;
        }
    }
    // 出环岛
    if(judge_island.right_island_type == 4)
    {
        static int16_t max_row = 0;
        static int16_t min_row = 0;
        if(judge_island.out_island_flag == false)
        {
            k = ((verge.left_inflection.column + 6) - LEFT_BOUNDARY_COLUMN) * 1.0f/(verge.left_inflection.row - WANT_START_ROW);
            b = verge.left_inflection.column - k * verge.left_inflection.row;
            judge_island.right_type4_k = k;
            judge_island.right_type4_b = b;
            max_row = (verge.left_inflection.row + judge_island.range_row) > WANT_START_ROW ? WANT_START_ROW : (verge.left_inflection.row + judge_island.range_row);
            min_row = (WANT_END_ROW + 8);
            linear_function_draw_line(image_buffer, max_row, min_row, k, b, 1);
            if(abs(verge.left_first_point.row - verge.left_inflection.row) < 25)
            {
                judge_island.out_island_flag = true;
            }
        }
        else
        {
            linear_function_draw_line(image_buffer, WANT_START_ROW, (WANT_END_ROW + 8), judge_island.right_type4_k, judge_island.right_type4_b, 1);
        }
        if(judge_island.out_island_flag == true) // 寻找尖点
        {
            static uint8_t righth4_shangjiandian_row = 0;
            static uint8_t righth4_shangjiandian_column = 0;
            static bool righth4_shangjiandian_flag = false;
            // 寻找尖点
            if(righth4_shangjiandian_flag == false)
            {
                int16_t max_row = limit_value((WANT_START_ROW - 50), WANT_START_ROW, (WANT_END_ROW + 10));
                int16_t min_row = (WANT_END_ROW + 10);
                int16_t max_column = limit_value((verge.left[verge.left_last_point.row].column + 4 * judge_island.range_column), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                int16_t min_column = limit_value((verge.left[verge.left_last_point.row].column + 5), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                bool found_point = false;
                for(row = max_row; row > min_row; row--)
                {
                    for(column = min_column; column < max_column; column++)
                    {
                        if(image_buffer[row - 1][column - 3] == WHITE && image_buffer[row - 1][column] == BLACK
                                && image_buffer[row][column - 3] == WHITE && image_buffer[row][column] == BLACK
                                && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                                )
                        {
                            righth4_shangjiandian_row = row;
                            righth4_shangjiandian_column = column;
                            righth4_shangjiandian_flag = true;
                            found_point = true;
                            break;
                        }
                    }
                    if(found_point)
                    {
                        break;
                    }
                }
                if(!found_point)
                {
                    righth4_shangjiandian_flag = false;
                }
            }
            else
            {
                int16_t max_row = limit_value((righth4_shangjiandian_row + judge_island.range_row), WANT_START_ROW, righth4_shangjiandian_row);
                int16_t min_row = limit_value((righth4_shangjiandian_row - judge_island.range_row), righth4_shangjiandian_row, WANT_END_ROW);
                int16_t max_column = limit_value((righth4_shangjiandian_column + judge_island.range_column), RIGHT_BOUNDARY_COLUMN, righth4_shangjiandian_column);
                int16_t min_column = limit_value((righth4_shangjiandian_column - judge_island.range_column), righth4_shangjiandian_column, LEFT_BOUNDARY_COLUMN);
                bool found_point = false;
                for(row = max_row; row > min_row; row--)
                {
                    for(column = min_column; column < max_column; column++)
                    {
                        if(image_buffer[row - 1][column - 3] == WHITE && image_buffer[row - 1][column] == BLACK
                                && image_buffer[row][column - 3] == WHITE && image_buffer[row][column] == BLACK
                                && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                                )
                        {
                            righth4_shangjiandian_row = row;
                            righth4_shangjiandian_column = column;
                            righth4_shangjiandian_flag = true;
                            found_point = true;
                            break;
                        }
                    }
                    if(found_point)
                    {
                        break;
                    }
                }
                if(!found_point)
                {
                    righth4_shangjiandian_flag = false;
                }
            }
            if(judge_island.out_island_flag == true && verge.left_first_point.row > (WANT_START_ROW - 10)
                && righth4_shangjiandian_row > (WANT_END_ROW + 18) && righth4_shangjiandian_column < (RIGHT_BOUNDARY_COLUMN - 16) // (48, 142)
                && righth4_shangjiandian_column > (START_MIDDLE_COLUMN + 20))
            {
                judge_island.out_island_flag = false;
                max_row = 0;
                min_row = 0;
                righth4_shangjiandian_row = 0;
                righth4_shangjiandian_column = 0;
                righth4_shangjiandian_flag = false;
                judge_island.right_type4_k = 0.0;
                judge_island.right_type4_b = 0.0;
                judge_island.right_island_type = 5;
            }
        }
        // 更新边界终点
        verge.left_last_point.row = (WANT_END_ROW + 8);
        verge.left_last_point.column = verge.left[(WANT_END_ROW + 8)].column;
        verge.left_last_point.column = limit_value(verge.left_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
        verge.left_last_point.flag = 1;
        for(uint8_t rrow = WANT_START_ROW; rrow >= WANT_END_ROW; rrow--) // 清除右边界, 切换为单边扫线
        {
            verge.right[rrow].column = RIGHT_BOUNDARY_COLUMN;
            verge.right[rrow].flag = 0;
        }
        if(judge_island.out_island_flag == true && verge.left_inflection.row < (WANT_END_ROW + 20) 
            && verge.left_inflection.column < (RIGHT_BOUNDARY_COLUMN - 8) && verge.left_inflection.column > (START_MIDDLE_COLUMN + 30)
            && abs(verge.left_inflection.column - verge.right_inflection.column) > 8)
        {
            judge_island.out_island_flag = false;
            max_row = 0;
            min_row = 0;
            judge_island.right_type4_k = 0.0;
            judge_island.right_type4_b = 0.0;
            judge_island.right_island_type = 5;
        }
    }
    // 防止再次进入环岛
    if(judge_island.right_island_type == 5)
    {
        static uint8_t righth5_shangjiandian_row = 0;
        static uint8_t righth5_shangjiandian_column = 0;
        static bool righth5_shangjiandian_flag = false;
        // 寻找尖点
        if(righth5_shangjiandian_flag == false)
        {
            int16_t max_row = limit_value((verge.left_last_point.row + (3 * judge_island.range_row)), WANT_START_ROW, verge.left_last_point.row);
            int16_t min_row = verge.left_last_point.row;
            int16_t max_column = limit_value((verge.left[verge.left_last_point.row].column + 4 * judge_island.range_column), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            int16_t min_column = limit_value((verge.left[verge.left_last_point.row].column + 5), RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            bool found_point = false;
            for(row = max_row; row > min_row; row--)
            {
                for(column = min_column; column < max_column; column++)
                {
                    if(image_buffer[row - 1][column - 3] == WHITE && image_buffer[row - 1][column] == BLACK
                            && image_buffer[row][column - 3] == WHITE && image_buffer[row][column] == BLACK
                            && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                            )
                    {
                        righth5_shangjiandian_row = row;
                        righth5_shangjiandian_column = column;
                        righth5_shangjiandian_flag = true;
                        found_point = true;
                        break;
                    }
                }
                if(found_point)
                {
                    break;
                }
            }
            if(!found_point)
            {
                righth5_shangjiandian_flag = false;
            }
        }
        else
        {
            int16_t max_row = limit_value((righth5_shangjiandian_row + judge_island.range_row), WANT_START_ROW, righth5_shangjiandian_row);
            int16_t min_row = limit_value((righth5_shangjiandian_row - judge_island.range_row), righth5_shangjiandian_row, WANT_END_ROW);
            int16_t max_column = limit_value((righth5_shangjiandian_column + judge_island.range_column), RIGHT_BOUNDARY_COLUMN, righth5_shangjiandian_column);
            int16_t min_column = limit_value((righth5_shangjiandian_column - judge_island.range_column), righth5_shangjiandian_column, LEFT_BOUNDARY_COLUMN);
            bool found_point = false;
            for(row = max_row; row > min_row; row--)
            {
                for(column = min_column; column < max_column; column++)
                {
                    if(image_buffer[row - 1][column - 3] == WHITE && image_buffer[row - 1][column] == BLACK
                            && image_buffer[row][column - 3] == WHITE && image_buffer[row][column] == BLACK
                            && image_buffer[row + 1][column - 3] == WHITE && image_buffer[row + 1][column] == WHITE && image_buffer[row + 1][column + 3] == WHITE
                            )
                    {
                        righth5_shangjiandian_row = row;
                        righth5_shangjiandian_column = column;
                        righth5_shangjiandian_flag = true;
                        found_point = true;
                        break;
                    }
                }
                if(found_point)
                {
                    break;
                }
            }
            if(!found_point)
            {
                righth5_shangjiandian_flag = false;
            }
        }
        if(righth5_shangjiandian_flag == true)
        {
            k = (RIGHT_BOUNDARY_COLUMN - righth5_shangjiandian_column) * 1.0f/(WANT_START_ROW - (righth5_shangjiandian_row));
            b = RIGHT_BOUNDARY_COLUMN - k * WANT_START_ROW;
            linear_function_draw_line(image_buffer, WANT_START_ROW, (righth5_shangjiandian_row - 1), k, b, 2);
            // 更新边界终点
            verge.right_last_point.row = (righth5_shangjiandian_row - 1);
            verge.right_last_point.column = verge.right[(righth5_shangjiandian_row - 1)].column;
            verge.right_last_point.column = limit_value(verge.right_last_point.column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            verge.right_last_point.flag = 1;
        }
        if(verge.left_first_point.flag == 1 && verge.right_first_point.flag == 1
            && verge.left_first_point.row > (WANT_START_ROW - 20) && verge.right_first_point.row > (WANT_START_ROW - 20)
            && verge.left_lose_line.row < (WANT_END_ROW + 20) && verge.right_lose_line.row < (WANT_END_ROW + 20))
        {
            gyro_element.island = 0;
            righth5_shangjiandian_row = 0;
            righth5_shangjiandian_column = 0;
            righth5_shangjiandian_flag = false;
            judge_island.right_island_type = 0;
            element_cooldown.island_cooldown = 50; // 完成右环岛后设置冷却时间
        }
    }
}

/**
 * @brief 判断元素
 * @param void
 * @return void
 * @note 判断元素，包括斑马线、坡道、避障、环岛、十字路口
 * @author YuYu
 */
void judge_element(void)
{
    /************************************************************
     *   元素判断顺序
     *   斑马线->十字路口->坡道->避障->环岛->直道
     *   已有元素判断时不进行其余元素判断，降低元素判断之间的干扰
     ************************************************************/
    if(integral_limit_flag == 1)
    {
        if(judge_crossroad.cross_flag == false && judge_crossroad.cross_left_flag == false && judge_crossroad.cross_right_flag == false
            && judge_ramp.flag == false
            && judge_roadblock.left_roadblock_flag == false && judge_roadblock.right_roadblock_flag == false
            && judge_island.left_island_type == 0 && judge_island.right_island_type == 0
            )
        {
            search_zebra_line(); // 搜索斑马线
        }
        if(judge_zebra_line.flag != 2
            && judge_ramp.flag == false
            && judge_roadblock.left_roadblock_flag == false && judge_roadblock.right_roadblock_flag == false
            && judge_island.left_island_type == 0 && judge_island.right_island_type == 0
            )
        {
            search_cross_road(); // 搜索十字路口
        }
        if(judge_zebra_line.flag != 2
            && judge_crossroad.cross_flag == false && judge_crossroad.cross_left_flag == false && judge_crossroad.cross_right_flag == false
            && judge_roadblock.left_roadblock_flag == false && judge_roadblock.right_roadblock_flag == false
            && judge_island.left_island_type == 0 && judge_island.right_island_type == 0
            && model_type == 0 && stop.misjudge_flag == false
            )
        {
            search_ramp(); // 搜索坡道
        }
        if(judge_zebra_line.flag != 2
            && judge_crossroad.cross_flag == false && judge_crossroad.cross_left_flag == false && judge_crossroad.cross_right_flag == false
            && judge_ramp.flag == false
            && judge_island.left_island_type == 0 && judge_island.right_island_type == 0
            && model_type == 0 && stop.misjudge_flag == false
            )
        {
            search_roadblock(); // 搜索避障
        }
        if(judge_zebra_line.flag != 2
            && judge_crossroad.cross_flag == false && judge_crossroad.cross_left_flag == false && judge_crossroad.cross_right_flag == false
            && judge_ramp.flag == false
            && judge_roadblock.left_roadblock_flag == false && judge_roadblock.right_roadblock_flag == false
            && judge_island.right_island_type == 0
            && model_type == 0 && stop.misjudge_flag == false
            )
        {
            search_left_island(); // 搜索左环岛
        }
        if(judge_zebra_line.flag != 2
            && judge_crossroad.cross_flag == false && judge_crossroad.cross_left_flag == false && judge_crossroad.cross_right_flag == false
            && judge_ramp.flag == false
            && judge_roadblock.left_roadblock_flag == false && judge_roadblock.right_roadblock_flag == false
            && judge_island.left_island_type == 0
            && model_type == 0 && stop.misjudge_flag == false
            )
        {
            search_right_island(); // 搜索右环岛
        }
    }
    element_reminder();
}

/**
 * @brief 元素提醒
 * @param void
 * @return void
 * @note 元素提醒，用于提醒元素
 * @author YuYu
 */
void element_reminder(void)
{
    // 识别元素
    if(judge_crossroad.cross_flag && !last_cross_flag)
    {
        printf("\033[1;34m[ELEMENT]\033[0m CROSS ROAD.\r\n");
        beep_enable(1);
        last_cross_flag = true;
    }
    else if(judge_crossroad.cross_left_flag && !last_cross_left_flag)
    {
        printf("\033[1;34m[ELEMENT]\033[0m LEFT_CROSS_ROAD.\r\n");
        beep_enable(1);
        last_cross_left_flag = true;
    }
    else if(judge_crossroad.cross_right_flag && !last_cross_right_flag)
    {
        printf("\033[1;34m[ELEMENT]\033[0m RIGHT_CROSS_ROAD.\r\n");
        beep_enable(1);        
        last_cross_right_flag = true;
    }
    else if(judge_island.left_island_type != 0 && judge_island.left_island_type != last_left_island_type)
    {
        printf("\033[1;34m[ELEMENT]\033[0m LEFT_ISLAND_TYPE = %d\r\n", judge_island.left_island_type);
        if(judge_island.left_island_type != 0)
        {
            beep_enable(1);
        }
        last_left_island_type = judge_island.left_island_type;
    }
    else if(judge_island.right_island_type != 0 && judge_island.right_island_type != last_right_island_type)
    {
        printf("\033[1;34m[ELEMENT]\033[0m RIGHT_ISLAND_TYPE = %d\r\n", judge_island.right_island_type);
        if(judge_island.right_island_type != 0)
        {
            beep_enable(1);
        }  
        last_right_island_type = judge_island.right_island_type;
    }   
    else if(judge_zebra_line.flag == 2 && judge_zebra_line.flag != last_zebra_line_flag)
    {
        printf("\033[1;34m[ELEMENT]\033[0m ZEBRA_LINE.\r\n");
        printf("\033[1;31m[SUCCESS]\033[0m 你问我为什么? What can I say? 周润 out!\r\n");
        // beep_enable(1);
        last_zebra_line_flag = judge_zebra_line.flag;
    }
    else if(judge_roadblock.left_roadblock_flag && !last_left_roadblock_flag)
    {
        printf("\033[1;34m[ELEMENT]\033[0m LEFT_ROADBLOCK.\r\n");
        beep_enable(1);
        last_left_roadblock_flag = true;
    }
    else if(judge_roadblock.right_roadblock_flag && !last_right_roadblock_flag)
    {
        printf("\033[1;34m[ELEMENT]\033[0m RIGHT_ROADBLOCK.\r\n");
        beep_enable(1);
        last_right_roadblock_flag = true;
    }
    else if(judge_ramp.flag && !last_ramp_flag)
    {
        printf("\033[1;34m[ELEMENT]\033[0m RAMP.\r\n");
        beep_enable(1);
        last_ramp_flag = true;
    }
    // 状态重置
    if(!judge_crossroad.cross_flag && !judge_crossroad.cross_left_flag && !judge_crossroad.cross_right_flag 
        && judge_island.left_island_type == 0 && judge_island.right_island_type == 0
        && judge_zebra_line.flag != 2 && !judge_roadblock.left_roadblock_flag 
        && !judge_roadblock.right_roadblock_flag && !judge_ramp.flag
        )
    {
        last_cross_flag = false;
        last_cross_left_flag = false;
        last_cross_right_flag = false;
        last_left_island_type = 0;
        last_right_island_type = 0;
        last_zebra_line_flag = 0;
        last_left_roadblock_flag = false;
        last_right_roadblock_flag = false;
        last_ramp_flag = false;
    }
    // 如果没有任何元素，关闭蜂鸣器
    if(!judge_crossroad.cross_flag && !judge_crossroad.cross_left_flag && !judge_crossroad.cross_right_flag 
        && judge_island.left_island_type == 0 && judge_island.right_island_type == 0
        && judge_zebra_line.flag != 2 && !judge_roadblock.left_roadblock_flag 
        && !judge_roadblock.right_roadblock_flag && !judge_ramp.flag
        )
    {
        beep_enable(0);
    }
}
/****************************** element ******************************/
