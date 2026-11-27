#include "get_image.h"

uint16_t* rgb_image;               // 定义RGB图像数据指针，用于存放摄像头采集到的RGB图像首地址
uint8_t* gray_image;               // 灰度图像数据指针，指向摄像头采集到的灰度图像缓冲区首地址
uint8_t image_buffer[ROW][COL];    // 图像缓冲区，用于存储处理后的图像数据
float adaption_threshold = 10.0;    // 自适应阈值
// 处理基本图像
struct boundary verge;
struct white_column longest_white_column;
struct middle_line center;
// 处理图像偏差
struct image_differ angle_line = 
{
    .far = 70,                  // 远端打角行 70
    .near = 100,                // 近端打角行 100
    .BLDC = 45,                 // BLDC打角行 45
    .differ_far = 0.0,
    .differ_near = 0.0,
    .differ_BLDC = 0.0,
    .differ_decision = 0.0,
    .last_differ_far = 0.0,
    .last_differ_near = 0.0,
    .last_differ_BLDC = 0.0
};

/****************************** display image ******************************/
/**
 * @brief 显示图像
 * @param void
 * @return void
 * @note 显示图像，包括边界线、赛道颜色、中线、want_start_row、want_end_row、Longest_White_Row、BLDC打搅行、远端打搅行
 * @author YuYu
 */
void display_image(void)
{
    uint8_t row, column;
    // 先清理非 ROI 区域
    for(row = 0; row < ROW; row++)
    {
        if(row > WANT_START_ROW || row < WANT_END_ROW)
        {
            for(column = 0; column < COL; column++) 
            {
                image_buffer[row][column] = BLACK;
            }
        }
    }
    // 扫描边界线
    for(row = WANT_END_ROW; row <= WANT_START_ROW; row++)
    {
        for(column = 0; column < COL; column++)
        {
            if(verge.left[row].flag == 1 && column == verge.left[row].column)
            {
                image_buffer[row][column] = WHITE; // 左边界线
                continue;
            }
            if(verge.right[row].flag == 1 && column == verge.right[row].column)
            {
                image_buffer[row][column] = WHITE; // 右边界线
                continue;
            }
            if(row >= center.middle_end_row && column == center.middle[row].column)
            {
                image_buffer[row][column] = WHITE; // 中线
                continue;
            }
            if(image_buffer[row][column] == WHITE)
            {
                image_buffer[row][column] = GRAY; // 处理赛道颜色
            }
        }
    }
    for(uint8_t i = 0; i < COL; i++)
    {
        image_buffer[WANT_START_ROW][i] = WHITE; // 显示 WANT_START_ROW
        image_buffer[WANT_END_ROW][i] = WHITE;   // 显示 WANT_END_ROW
    }
    for(uint8_t i = 40; i < 120; i++)
    {
        image_buffer[longest_white_column.row][i] = WHITE; // 显示 want_end_row 行, 即 Longest_White_Row
        image_buffer[angle_line.BLDC][i] = WHITE; // 显示BLDC打搅行
        image_buffer[angle_line.far][i] = WHITE; // 显示远端打搅行
    }
    for(uint8_t i = WANT_END_ROW; i <= WANT_START_ROW; i++)
    {
        image_buffer[i][longest_white_column.column] = WHITE; // 显示 longest_white_column.column
    }
    // 色块检测 ROI 区域矩形框
    #if USE_GRAY_DETECT == 1
    int16_t roi_top = DETECT_MIN_ROW/2;
    int16_t roi_bottom = DETECT_MAX_ROW/2;
    int16_t roi_left = DETECT_MIN_COLUMN/2;
    int16_t roi_right = DETECT_MAX_COLUMN/2;
    roi_top = roi_top < 0 ? 0 : (roi_top >= ROW ? ROW - 1 : roi_top);
    roi_bottom = roi_bottom < 0 ? 0 : (roi_bottom >= ROW ? ROW - 1 : roi_bottom);
    roi_left = roi_left < 0 ? 0 : (roi_left >= COL ? COL - 1 : roi_left);
    roi_right = roi_right < 0 ? 0 : (roi_right >= COL ? COL - 1 : roi_right);
    for(int16_t c = roi_left; c <= roi_right; c++)
    {
        image_buffer[roi_top][c] = WHITE;
        image_buffer[roi_bottom][c] = WHITE;
    }
    for(int16_t r = roi_top; r <= roi_bottom; r++)
    {
        image_buffer[r][roi_left] = WHITE;
        image_buffer[r][roi_right] = WHITE;
    }
    // 绘制红色色块矩形框 (灰度坐标)
    if(stop.detect_RGB_red == true || model_control.turn_flag == true || model_control.return_flag == true)
    {
        int16_t bx = bottom_left_x/2;
        int16_t by = bottom_left_y/2;
        int16_t bw = red_block_w/2;
        int16_t bh = red_block_h/2;
        int16_t block_top = by - bh;
        int16_t block_bottom = by;
        int16_t block_left = bx;
        int16_t block_right = bx + bw;
        block_top = block_top < 0 ? 0 : (block_top >= ROW ? ROW - 1 : block_top);
        block_bottom = block_bottom < 0 ? 0 : (block_bottom >= ROW ? ROW - 1 : block_bottom);
        block_left = block_left < 0 ? 0 : (block_left >= COL ? COL - 1 : block_left);
        block_right = block_right < 0 ? 0 : (block_right >= COL ? COL - 1 : block_right);
        for(int16_t c = block_left; c <= block_right; c++)
        {
            image_buffer[block_top][c] = WHITE;
            image_buffer[block_bottom][c] = WHITE;
        }
        for(int16_t r = block_top; r <= block_bottom; r++)
        {
            image_buffer[r][block_left] = WHITE;
            image_buffer[r][block_right] = WHITE;
        }
        image_buffer[model_control.block_y][model_control.block_x] = WHITE;
    }
    #endif
}
/****************************** display image ******************************/

/****************************** deal basic image ******************************/
/**
 * @brief 查找左边界
 * @param row 行
 * @param start_column 开始列
 * @param end_column 结束列
 * @return void
 * @note 查找左边界，从 start_column 开始查找，直到 end_column
 * @author YuYu
 */
void search_left_edge(uint8_t row, uint8_t start_column, uint8_t end_column)
{
    uint8_t column;
    uint8_t assist_column;
    // 区分 assist_column 的查找方式
    if(row < (WANT_START_ROW - 1) && row >= WANT_END_ROW && verge.left[row + 1].flag == 1)
    {
        int16_t base_column = verge.left[row + 1].column;
        if(row + 2 < ROW && verge.left[row + 2].flag == 1)
        {
            int16_t diff = 2 * abs(base_column - verge.left[row + 2].column);
            assist_column = base_column + diff;
        } 
        else
        {
            assist_column = base_column;
        }
        assist_column = limit_value(assist_column, (base_column + CHANGE_COLUMN), (base_column - CHANGE_COLUMN));
    }
    else
    {
        assist_column = verge.left[row + 1].column + CHANGE_COLUMN;
    }
    // 限制查找范围
    start_column = limit_value(start_column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
    end_column = limit_value(end_column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
    assist_column = limit_value(assist_column, start_column, end_column);
    // 查找左边界
    if(start_column > end_column)
    {
        if(row == WANT_START_ROW)
        {
            // 从 start_column 开始查找左边界
            for(column = start_column; column > end_column; column--)
            {
                // 增强的边界检测，考虑更多像素
                if(image_buffer[row][column] == WHITE && 
                    image_buffer[row][column - 1] == BLACK && 
                    (column - 2 >= LEFT_BOUNDARY_COLUMN ? image_buffer[row][column - 2] == BLACK : true))
                {
                    verge.left[row].column = column;
                    verge.left[row].flag = 1;
                    return;
                }
            }
        }
        else
        {
            // 从 assist_column 开始查找左边界
            for(column = assist_column; column > end_column && column >= LEFT_BOUNDARY_COLUMN; column--)
            {
                if(image_buffer[row][column] == WHITE && 
                   image_buffer[row][column - 1] == BLACK && 
                   (column - 2 >= LEFT_BOUNDARY_COLUMN ? image_buffer[row][column - 2] == BLACK : true))
                {
                    verge.left[row].column = column;
                    verge.left[row].flag = 1;
                    return;
                }
            }
            // 从 assist_column + 1 开始查找左边界
            for(column = assist_column + 1; column < start_column && column <= RIGHT_BOUNDARY_COLUMN && column >= LEFT_BOUNDARY_COLUMN; column++)
            {
                if(image_buffer[row][column] == WHITE && 
                   image_buffer[row][column - 1] == BLACK && 
                   (column - 2 >= LEFT_BOUNDARY_COLUMN ? image_buffer[row][column - 2] == BLACK : true))
                {
                    verge.left[row].column = column;
                    verge.left[row].flag = 1;
                    return;
                }
            }
        }
    }
    // 如果未找到边界，设置为无效
    verge.left[row].flag = 0;
}

/**
 * @brief 查找右边界
 * @param row 行
 * @param start_column 开始列
 * @param end_column 结束列
 * @return void
 * @note 查找右边界，从 start_column 开始查找，直到 end_column
 * @author YuYu
 */
void search_right_edge(uint8_t row, uint8_t start_column, uint8_t end_column) // -->
{
    uint8_t column;
    uint8_t assist_column;
    // 区分 assist_column 的查找方式
    if(row < (WANT_START_ROW - 1) && row >= WANT_END_ROW && verge.right[row + 1].flag == 1)
    {
        int16_t base_column = verge.right[row + 1].column;
        if(row + 2 < ROW && verge.right[row + 2].flag == 1) 
        {
            int16_t diff = 2 * abs(base_column - verge.right[row + 2].column);
            assist_column = base_column - diff;
        } 
        else
        {
            assist_column = base_column;
        }
        assist_column = limit_value(assist_column, (base_column + CHANGE_COLUMN), (base_column - CHANGE_COLUMN));
    }
    else
    {
        assist_column = verge.right[row + 1].column - CHANGE_COLUMN;
    }
    // 限制查找范围
    start_column = limit_value(start_column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
    end_column = limit_value(end_column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
    assist_column = limit_value(assist_column, start_column, end_column);
    // 查找右边界
    if(start_column < end_column)
    {
        if(row == WANT_START_ROW)
        {
            // 从 start_column 开始查找右边界 -->
            for(column = start_column; column < end_column && column < RIGHT_BOUNDARY_COLUMN; column++)
            {
                // 增强的边界检测，考虑更多像素
                if(image_buffer[row][column] == WHITE && 
                   image_buffer[row][column + 1] == BLACK && 
                   (column + 2 <= RIGHT_BOUNDARY_COLUMN ? image_buffer[row][column + 2] == BLACK : true))
                {
                    verge.right[row].column = column;
                    verge.right[row].flag = 1;
                    return;
                }
            }
        }
        else
        {
            // 从 assist_column 开始查找右边界 -->
            for(column = assist_column; column < end_column && column < RIGHT_BOUNDARY_COLUMN; column++)
            {
                if(image_buffer[row][column] == WHITE && 
                   image_buffer[row][column + 1] == BLACK && 
                   (column + 2 <= RIGHT_BOUNDARY_COLUMN ? image_buffer[row][column + 2] == BLACK : true))
                {
                    verge.right[row].column = column;
                    verge.right[row].flag = 1;
                    return;
                }
            }
            // 从 assist_column - 1 开始查找右边界 <--
            for(column = assist_column - 1; column > start_column && column <= RIGHT_BOUNDARY_COLUMN && column >= LEFT_BOUNDARY_COLUMN; column--)
            {
                if(image_buffer[row][column] == WHITE && 
                   image_buffer[row][column + 1] == BLACK && 
                   (column + 2 <= RIGHT_BOUNDARY_COLUMN ? image_buffer[row][column + 2] == BLACK : true))
                {
                    verge.right[row].column = column;
                    verge.right[row].flag = 1;
                    return;
                }
            }
        }
    }
    // 如果未找到边界，设置为无效
    verge.right[row].flag = 0;
}

/**
 * @brief 查找左边界拐点
 * @param start_row 开始行
 * @param end_row 结束行
 * @return void
 * @note 查找左边界的拐点，即左边界列值最大的点，如果拐点位于结束行，则标记为无效
 * @author YuYu
 */
void search_left_inflection(uint8_t start_row, uint8_t end_row)
{
    uint8_t row;
    uint8_t max_column = LEFT_BOUNDARY_COLUMN;
    bool found_valid = false;
    // 从 start_row 开始查找左边界拐点
    for(row = start_row; row >= end_row; row--)
    {
        // 只考虑有效边界
        if(verge.left[row].flag == 1 && verge.left[row].column >= max_column)
        {
            max_column = verge.left[row].column;
            verge.left_inflection.row = row;
            verge.left_inflection.column = verge.left[row].column;
            found_valid = true;
        }
    }
    // 只有当找到有效边界且拐点不在结束行时，才标记为有效
    verge.left_inflection.flag = (found_valid && verge.left_inflection.row != end_row) ? 1 : 0;
}

/**
 * @brief 查找右边界拐点
 * @param start_row 开始行
 * @param end_row 结束行
 * @return void
 * @note 查找右边界的拐点，即右边界列值最小的的点，如果拐点位于结束行，则标记为无效
 * @author YuYu
 */
void search_right_inflection(uint8_t start_row, uint8_t end_row)
{
    uint8_t row;
    uint8_t min_column = RIGHT_BOUNDARY_COLUMN;
    bool found_valid = false;
    // 从 start_row 开始查找右边界拐点
    for(row = start_row; row >= end_row; row--)
    {
        // 只考虑有效边界
        if(verge.right[row].flag == 1 && verge.right[row].column <= min_column)
        {
            min_column = verge.right[row].column;
            verge.right_inflection.row = row;
            verge.right_inflection.column = verge.right[row].column;
            found_valid = true;
        }
    }
    // 只有当找到有效边界且拐点不在结束行时，才标记为有效
    verge.right_inflection.flag = (found_valid && verge.right_inflection.row != end_row) ? 1 : 0;
}

/**
 * @brief 查找最长白行
 * @param start_row 开始行
 * @param end_row 结束行
 * @param column 列
 * @return void
 * @note 查找最长白行，从 start_row 开始查找，直到 end_row
 * @author YuYu
 */
void search_longest_row(uint8_t start_row, uint8_t end_row, uint8_t column)
{
    uint8_t row;
    uint8_t times = 0;
    longest_white_column.IMAGE_CBH = 0;
    // 现在查找范围
    start_row = (start_row > WANT_START_ROW) ? WANT_START_ROW : start_row;
    end_row = (end_row < WANT_END_ROW) ? WANT_END_ROW : end_row;
    // 从 start_row 开始查找最长白行
    for(row = start_row; row >= end_row; row--)
    {
        if(row + 1 >= ROW) // 检查是否超出边界
        { 
            continue;
        }
        longest_white_column.IMAGE_CBH = calc_diff(gray_image[(row + 1) * COL + column], gray_image[row * COL + column]);
        // 如果当前行与下一行的灰度值差异小于阈值，说明当前行是白行
        if(longest_white_column.IMAGE_CBH < -CBH_THRESHOLD)
        {
            longest_white_column.down[column].row = WANT_START_ROW;
            longest_white_column.down[column].flag = 0;
            break;
        }
        // 如果当前行与下一行的灰度值差异大于阈值，说明当前行是黑行
        if(longest_white_column.IMAGE_CBH > CBH_THRESHOLD)
        {
            if(times != 0)
            {
                longest_white_column.llast_down[column].row = longest_white_column.last_down[column].row;
                longest_white_column.last_down[column].row = longest_white_column.down[column].row;
            }
            longest_white_column.down[column].row = row;
            longest_white_column.down[column].flag = 1;
            if((abs(longest_white_column.last_down[column].row - longest_white_column.down[column].row) > 5) && times != 0)
            {
                longest_white_column.last_down[column].row = longest_white_column.llast_down[column].row;
                longest_white_column.down[column].row = WANT_START_ROW;
                longest_white_column.down[column].flag = 0;
            }
            times++;
            break;
        }
    }
}

/**
 * @brief 计算灰度值差异
 * @param x 灰度值1
 * @param y 灰度值2
 * @return int16_t 灰度值差异
 * @note 计算灰度值差异，返回差异的 7 位整数表示
 * @author YuYu
 */
int16_t calc_diff(int16_t x, int16_t y) // 约放大128倍
{
    if(x == 0 && y == 0)
    {
        return 0;
    }
    return (((x - y) << 7) / (x + y));
}
/****************************** deal basic image ******************************/

/****************************** calculate image differ ******************************/
/**
 * @brief 获取中线
 * @param void
 * @return void
 * @note 获取当前图像的中线，根据最近的有效边界点进行预测
 * @author YuYu
 */
void obtain_middle_line(void)
{
    uint8_t row;
    int16_t left_val = 0, right_val = 0;
    int16_t left_prev1 = 0, left_prev2 = 0;
    int16_t right_prev1 = 0, right_prev2 = 0;
    uint8_t left_lost = 0;
    uint8_t right_lost = 0;
    center.middle_end_row = (uint8_t)find_min(verge.left_last_point.row, verge.right_last_point.row);
    for(row = WANT_START_ROW; row >= center.middle_end_row; row--)
    {
        if(verge.left[row].flag == 1 && verge.right[row].flag == 1)
        {
            center.middle[row].column = (uint8_t)((verge.left[row].column + verge.right[row].column) >> 1);
            center.middle[row].flag = 1;
            left_prev2 = left_prev1;
            left_prev1 = verge.left[row].column;
            right_prev2 = right_prev1;
            right_prev1 = verge.right[row].column;
            left_lost = 0;
            right_lost = 0;
        }
        else if(verge.left[row].flag == 1)
        {
            right_lost++;
            if(right_lost <= EXTRA_ROWS && right_prev1 > 0)
            {
                right_val = right_prev1 + (right_prev1 - right_prev2);
                right_val = limit_value(right_val, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                center.middle[row].column = (uint8_t)((verge.left[row].column + right_val) >> 1);
            }
            else if(right_lost <= EXTRA_ROWS + BLEND_ROWS && right_prev1 > 0)
            {
                right_val = right_prev1 + (right_prev1 - right_prev2);
                right_val = limit_value(right_val, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                uint8_t step = right_lost - EXTRA_ROWS;
                float weight = 1.0f - (float)step / (float)(BLEND_ROWS + 1);
                float extra_center = (verge.left[row].column + right_val) / 2.0f;
                float fallback_center = (verge.left[row].column + RIGHT_BOUNDARY_COLUMN) / 2.0f;
                center.middle[row].column = (uint8_t)(weight * extra_center + (1.0f - weight) * fallback_center + 0.5f);
            }
            else
            {
                center.middle[row].column = (uint8_t)((verge.left[row].column + RIGHT_BOUNDARY_COLUMN) >> 1);
            }
            center.middle[row].flag = 1;
            left_prev2 = left_prev1;
            left_prev1 = verge.left[row].column;
            left_lost = 0;
        }
        else if(verge.right[row].flag == 1)
        {
            left_lost++;
            if(left_lost <= EXTRA_ROWS && left_prev1 > 0)
            {
                left_val = left_prev1 + (left_prev1 - left_prev2);
                left_val = limit_value(left_val, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                center.middle[row].column = (uint8_t)((left_val + verge.right[row].column) >> 1);
            }
            else if(left_lost <= EXTRA_ROWS + BLEND_ROWS && left_prev1 > 0)
            {
                left_val = left_prev1 + (left_prev1 - left_prev2);
                left_val = limit_value(left_val, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
                uint8_t step = left_lost - EXTRA_ROWS;
                float weight = 1.0f - (float)step / (float)(BLEND_ROWS + 1);
                float extra_center = (left_val + verge.right[row].column) / 2.0f;
                float fallback_center = (LEFT_BOUNDARY_COLUMN + verge.right[row].column) / 2.0f;
                center.middle[row].column = (uint8_t)(weight * extra_center + (1.0f - weight) * fallback_center + 0.5f);
            }
            else
            {
                center.middle[row].column = (uint8_t)((verge.right[row].column + LEFT_BOUNDARY_COLUMN) >> 1);
            }
            center.middle[row].flag = 1;
            right_prev2 = right_prev1;
            right_prev1 = verge.right[row].column;
            right_lost = 0;
        }
        center.middle[row].column = limit_value(center.middle[row].column, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
    }
    smooth_middle_line();
    deal_image_differ();
}

/**
 * @brief Savitzky-Golay多项式平滑中线
 * @param void
 * @return void
 * @note 使用5点2阶Savitzky-Golay滤波平滑中线，响应速度更快
 * @author YuYu
 */
void smooth_middle_line(void)
{
    uint8_t row;
    uint8_t original_middle[ROW];
    // 保存原始中线值
    for(row = 0; row < ROW; row++)
    {
        original_middle[row] = center.middle[row].column;
    }
    // 从 center.middle_end_row 开始平滑中线
    for(row = center.middle_end_row; row <= WANT_START_ROW; row++)
    {
        if(center.middle[row].flag != 1)
        {
            continue;
        }
        // 5点2阶Savitzky-Golay系数: [-3, 12, 17, 12, -3] / 35
        if(row - 2 >= center.middle_end_row && row + 2 <= WANT_START_ROW &&
           center.middle[row - 2].flag == 1 && center.middle[row - 1].flag == 1 &&
           center.middle[row + 1].flag == 1 && center.middle[row + 2].flag == 1)
        {
            float smoothed = (-3.0f * original_middle[row - 2] +
                              12.0f * original_middle[row - 1] +
                              17.0f * original_middle[row] +
                              12.0f * original_middle[row + 1] +
                              -3.0f * original_middle[row + 2]) / 35.0f;
            center.middle[row].column = (uint8_t)(smoothed + 0.5f);
        }
        // 边界3点平均平滑
        else if(row - 1 >= center.middle_end_row && row + 1 <= WANT_START_ROW &&
                center.middle[row - 1].flag == 1 && center.middle[row + 1].flag == 1)
        {
            float smoothed = (original_middle[row - 1] + 
                             original_middle[row] + 
                             original_middle[row + 1]) / 3.0f;
            center.middle[row].column = (uint8_t)(smoothed + 0.5f);
        }
        else
        {
            center.middle[row].column = original_middle[row];
        }
    }
}

/**
 * @brief 计算图像误差
 * @param void
 * @return void
 * @note 计算图像误差，包括远端误差和 BLDC 误差
 * @author YuYu
 */
void deal_image_differ(void)
{
    // calculate differ far
    calculate_far_differ(angle_line.far); // far
    calculate_near_differ(angle_line.near); // near
    calculate_BLDC_differ(angle_line.BLDC); // BLDC
    // calculate decision differ
    angle_line.differ_decision = angle_line.differ_far;
    // image differ PID
    float image_differ_err = differ_Kp1[SET_DIFFER_TYPE] * (angle_line.differ_far * fabs(angle_line.differ_far))/AMPLIFICATION_DIFFER 
                           + differ_Kp2[SET_DIFFER_TYPE] * angle_line.differ_far
                           + differ_Kd[SET_DIFFER_TYPE] * (angle_line.differ_far - angle_line.last_differ_far);
    image_differ_err = image_differ_err + (differ_Kp3[SET_DIFFER_TYPE] * angle_line.differ_near);
    angle_line.differ_far = limit_value(image_differ_err, IMAGE_DIFFER_MAX, -IMAGE_DIFFER_MAX);
}

/**
 * @brief 计算远端误差
 * @param far_angle_line 远端行
 * @note void
 * @return void
 * @author YuYu
 */
void calculate_far_differ(uint8_t far_angle_line)
{
    uint8_t far_line = ((far_angle_line + 2) - (2 * SET_DIFFER_TYPE));
    #if 1 == USE_SPEED_DECISION
    far_line = far_line - (uint8_t)((set_speed - 80)/5);
    far_line = limit_value(far_line, (far_line + 4), (far_line - 16));
    #endif
    if(judge_ramp.flag == true) // 坡道
    {
        angle_line.differ_far = START_MIDDLE_COLUMN - center.middle[RAMP_DIFFER_ROW].column;
        angle_line.differ_far = limit_value(angle_line.differ_far, RAMP_MAX_DIFFER, -RAMP_MAX_DIFFER);
    }
    else if(last_model_type == 1) // 左转弯
    {
        float prevent_differ = 0.0f;
        float speed_differ = (set_speed - 80) * 0.1f; // 根据速度调整基础偏差
        speed_differ = limit_value(speed_differ, MAX_DIFFER_BIAS * 0.3f, -MAX_DIFFER_BIAS * 0.3f);
        if(model_control.turn_flag == true)
        {
            float modle_differ = (model_control.block_x - START_MIDDLE_COLUMN) * 0.3f;
            modle_differ = limit_value(modle_differ, MAX_DIFFER_BIAS * 0.5f, -MAX_DIFFER_BIAS * 0.5f);
            if(model_control.middle_end_row >= 45 && modle_differ < -30) // 模型在左侧
            {
                prevent_differ = (MAX_DIFFER_BIAS + speed_differ) + modle_differ;
            }
            else if(model_control.middle_end_row >= 45 && modle_differ > 30) // 模型在右侧
            {
                prevent_differ = (MAX_DIFFER_BIAS + speed_differ) - modle_differ;
            }
            else
            {
                prevent_differ = (MAX_DIFFER_BIAS + speed_differ * 1.2);
            }
        }
        else if(model_control.return_flag == true)
        {
            float modle_differ = (model_control.block_x - START_MIDDLE_COLUMN) * 0.5f;
            modle_differ = limit_value(modle_differ, MAX_DIFFER_BIAS, -MAX_DIFFER_BIAS);
            if(model_control.middle_end_row >= 45 && modle_differ < -30) // 模型在左侧
            {
                prevent_differ = -(MAX_DIFFER_BIAS + speed_differ) - modle_differ;
            }
            else if(model_control.middle_end_row >= 45 && modle_differ > 30) // 模型在右侧
            {
                prevent_differ = -(MAX_DIFFER_BIAS + speed_differ) - modle_differ;
            }
            else
            {
                prevent_differ = -(MAX_DIFFER_BIAS + speed_differ * 1.2);
            }
        }
        angle_line.differ_far = 0.9f * prevent_differ + 0.1f * angle_line.last_differ_far;
        angle_line.differ_far = limit_value(angle_line.differ_far, FAR_DIFFER_MAX, -FAR_DIFFER_MAX);
        angle_line.last_differ_far = angle_line.differ_far;
    }
    else if(last_model_type == 2) // 右转弯
    {
        float prevent_differ = 0.0f;
        float speed_differ = (set_speed - 80) * 0.1f; // 根据速度调整基础偏差
        speed_differ = limit_value(speed_differ, MAX_DIFFER_BIAS * 0.3f, -MAX_DIFFER_BIAS * 0.3f);
        if(model_control.turn_flag == true)
        {
            float modle_differ = (model_control.block_x - START_MIDDLE_COLUMN) * 0.3f;
            modle_differ = limit_value(modle_differ, MAX_DIFFER_BIAS * 0.5f, -MAX_DIFFER_BIAS * 0.5f);
            if(model_control.middle_end_row >= 45 && modle_differ > 30) // 模型在右侧
            {
                prevent_differ = -(MAX_DIFFER_BIAS + speed_differ) - modle_differ;
            }
            else if(model_control.middle_end_row >= 45 && modle_differ < -30) // 模型在左侧
            {
                prevent_differ = -(MAX_DIFFER_BIAS + speed_differ) - modle_differ;
            }
            else
            {
                prevent_differ = -(MAX_DIFFER_BIAS + speed_differ * 1.2);
            }
        }
        else if(model_control.return_flag == true)
        {
            float modle_differ = (model_control.block_x - START_MIDDLE_COLUMN) * 0.5f;
            modle_differ = limit_value(modle_differ, MAX_DIFFER_BIAS, -MAX_DIFFER_BIAS);
            if(model_control.middle_end_row >= 45 && modle_differ > 30) // 模型在右侧
            {
                prevent_differ = (MAX_DIFFER_BIAS + speed_differ) - modle_differ;
            }
            else if(model_control.middle_end_row >= 45 && modle_differ < -30) // 模型在左侧
            {
                prevent_differ = (MAX_DIFFER_BIAS + speed_differ) + modle_differ;
            }
            else
            {
                prevent_differ = (MAX_DIFFER_BIAS + speed_differ * 1.2);
            }
        }
        angle_line.differ_far = 0.9f * prevent_differ + 0.1f * angle_line.last_differ_far;
        angle_line.differ_far = limit_value(angle_line.differ_far, FAR_DIFFER_MAX, -FAR_DIFFER_MAX);
        angle_line.last_differ_far = angle_line.differ_far;
    }
    else
    {
        int16_t i = 0;
        uint8_t far_count = 0;
        float far_sum_differ = 0.0;
        // 从 far_line 开始查找远端误差
        for(i = far_line; i >= (far_line - 4); i--) // 连续五行计算偏差
        {
            far_count++;
            if(i <= center.middle_end_row)
            {
                far_sum_differ += (START_MIDDLE_COLUMN - center.middle[center.middle_end_row].column);
            }
            else
            {
                far_sum_differ += (START_MIDDLE_COLUMN - center.middle[i].column);
            }
        }
        // 计算远端误差
        float now_differ = far_sum_differ/far_count;
        angle_line.differ_far = 0.8 * now_differ + 0.2 * angle_line.last_differ_far; // 互补滤波
        if(judge_island.left_island_type == 2 || judge_island.left_island_type == 3 || judge_island.left_island_type == 4)
        {
            angle_line.differ_far = angle_line.differ_far > 16 ? angle_line.differ_far : 16;
        }
        else if(judge_island.right_island_type == 2 || judge_island.right_island_type == 3 || judge_island.right_island_type == 4)
        {
            angle_line.differ_far = angle_line.differ_far < -16 ? angle_line.differ_far : -16;
        }
        angle_line.differ_far = limit_value(angle_line.differ_far, FAR_DIFFER_MAX, -FAR_DIFFER_MAX);
        angle_line.last_differ_far = angle_line.differ_far;
    }
    // printf("far_line = %d\r\n", far_line);
    // printf("angle_line.differ_far = %f\r\n", angle_line.differ_far);
}

/**
 * @brief 计算近端误差
 * @param near_angle_line 近端行
 * @note void
 * @return void
 * @author YuYu
 */
void calculate_near_differ(uint8_t near_angle_line)
{
    uint8_t near_line = near_angle_line + 1;
    if(judge_ramp.flag == true) // 坡道
    {
        angle_line.differ_near = 0.0f;
    }
    else if(last_model_type == 1) // 左转弯
    {
        angle_line.differ_near = 0.0f;
    }
    else if(last_model_type == 2) // 右转弯
    {
        angle_line.differ_near = 0.0f;
    }
    else
    {
        int16_t i = 0;
        uint8_t near_count = 0;
        float near_sum_differ = 0.0;
        // 从 near_line 开始查找近端误差
        for(i = near_line; i >=  (near_line - 2); i--)
        {
            near_count++;
            if(i <= center.middle_end_row)
            {
                near_sum_differ += (START_MIDDLE_COLUMN - center.middle[center.middle_end_row].column);
            }
            else
            {
                near_sum_differ += (START_MIDDLE_COLUMN - center.middle[i].column);
            }
        }
        // 计算近端误差
        float now_differ = near_sum_differ/near_count;
        angle_line.differ_near = 0.8 * now_differ + 0.2 * angle_line.last_differ_near; // 互补滤波
        angle_line.differ_near = limit_value(angle_line.differ_near, NEAR_DIFFER_MAX, -NEAR_DIFFER_MAX);
        angle_line.last_differ_near = angle_line.differ_near;
    }
}

/**
 * @brief 计算 BLDC 误差
 * @param BLDC_angle_line BLDC 角度
 * @note void
 * @return void
 * @author YuYu
 */
void calculate_BLDC_differ(uint8_t BLDC_angle_line)
{
    uint8_t BLDC_line = BLDC_angle_line + 1;
    int16_t i = 0;
    uint8_t BLDC_count = 0;
    float BLDC_sum_differ = 0.0;
    // 从 BLDC_line 开始查找 BLDC 误差
    for(i = BLDC_line; i >=  (BLDC_line - 2); i--)
    {
        BLDC_count++;
        if(i <= center.middle_end_row)
        {
            BLDC_sum_differ += (START_MIDDLE_COLUMN - center.middle[center.middle_end_row].column);
        }
        else
        {
            BLDC_sum_differ += (START_MIDDLE_COLUMN - center.middle[i].column);
        }
    }
    // 计算 BLDC 误差
    float now_differ = BLDC_sum_differ/BLDC_count;
    angle_line.differ_BLDC = 0.8 * now_differ + 0.2 * angle_line.last_differ_BLDC; // 互补滤波
    angle_line.differ_BLDC = limit_value(angle_line.differ_BLDC, BLDC_DIFFER_MAX, -BLDC_DIFFER_MAX);
    angle_line.last_differ_BLDC = angle_line.differ_BLDC;
}
/****************************** calculate image differ ******************************/

/****************************** function ******************************/
/**
 * @brief 计算直线
 * @param x 纵坐标
 * @param k 斜率
 * @param b 截距
 * @return 直线方程
 * @note 计算直线方程，返回 y = k * x + b
 * @author YuYu
 */
int16_t add_line(int16_t x, float k, float b)
{
    return (int16_t)(k * x + b);
}

/**
 * @brief 绘制直线
 * @param image_buffer 图像缓冲区
 * @param start_row 开始行
 * @param end_row 结束行
 * @param k 斜率
 * @param b 截距
 * @param type 直线类型
 * @return void
 * @note 绘制直线，根据 type 绘制左边界或右边界
 * @author YuYu
 */
void linear_function_draw_line(uint8_t image_buffer[][COL], uint8_t start_row, uint8_t end_row, float k, float b, uint8_t type)
{
    if(type == 1) // 直接更新左边界
    {
        for(uint8_t i = start_row; i >= end_row; i--)
        {
            int16_t col = add_line(i, k, b);
            col = limit_value(col, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            verge.left[i].column = (uint8_t)col;
            verge.left[i].flag = 1; // 设置有效标志
        }
    }
    else if(type == 2) // 直接更新右边界
    {
        for(uint8_t i = start_row; i >= end_row; i--)
        {
            int16_t col = add_line(i, k, b);
            // 限制列坐标在有效范围内
            col = limit_value(col, RIGHT_BOUNDARY_COLUMN, LEFT_BOUNDARY_COLUMN);
            // 直接更新边界信息
            verge.right[i].column = (uint8_t)col;
            verge.right[i].flag = 1; // 设置有效标志
        }
    }
}

/**
 * @brief 查找黑白过渡
 * @param image_buffer 图像缓冲区
 * @param start_row 开始行
 * @param end_row 结束行
 * @param target_column 目标列
 * @return void
 * @note 查找黑白过渡，统计黑白像素数量
 * @author YuYu
 */
uint8_t search_black_white_transition(uint8_t image_buffer[][COL], uint8_t start_row, uint8_t end_row, uint8_t target_column, uint8_t type)
{
    uint8_t count = 0;
    if(type == 1)
    {
        uint8_t white_count = 0;
        for(uint8_t i = start_row; i > end_row; i--)
        {
            if(image_buffer[i][target_column] == WHITE)
            {
                white_count++;
            }
            if(white_count != 0 && image_buffer[i][target_column] == BLACK && image_buffer[(i - 1)][target_column] == BLACK)
            {
                break;
            }
        }
        count = white_count;
    }
    else if(type == 2)
    {
        uint8_t black_count = 0;
        for(uint8_t i = start_row; i > end_row; i--)
        {
            if(image_buffer[i][target_column] == BLACK)
            {
                black_count++;
            }
            if(black_count != 0 && image_buffer[i][target_column] == WHITE && image_buffer[(i - 1)][target_column] == WHITE)
            {
                break;
            }
        }
        count = black_count;
    }
    return count;
}

/**
 * @brief 查找最大值
 * @param a 数值1
 * @param b 数值2
 * @return int16_t 最大值
 * @note 查找最大值，返回 a 和 b 中较大的那个
 * @author YuYu
 */
int16_t find_max(int16_t a, int16_t b)
{
    int16_t temp = 0;
    temp = (a > b) ? a : b;
    return temp;
}

/**
 * @brief 查找最小值
 * @param a 数值1
 * @param b 数值2
 * @return int16_t 最小值
 * @note 查找最小值，返回 a 和 b 中较小的那个
 * @author YuYu
 */
int16_t find_min(int16_t a, int16_t b)
{
    int16_t temp = 0;
    temp = (a < b) ? a : b;
    return temp;
}

/**
 * @brief 限制值范围
 * @param value 值
 * @param max_value 最大值
 * @param min_value 最小值
 * @return float 限制后的值
 * @note 限制值范围，返回 value 超出范围时的边界值
 * @author YuYu
 */
float limit_value(float value, float max_value, float min_value)
{
    if(value > max_value) value = max_value;
    if(value < min_value) value = min_value;
    return value;
}
/****************************** function ******************************/
