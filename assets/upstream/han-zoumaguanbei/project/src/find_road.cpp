#include "zf_common_headfile.h"
#include "classifier.h"

using namespace std;
using namespace cv;
extern void *model;
extern Mat square;

extern int i;
extern int j;
extern int left_line[1000][2];
extern int right_line[1000][2];
extern int mid_line_from_left[1000][2];
extern int mid_line_from_right[1000][2];
int left_corner[100][2];
int right_corner[100][2];
extern int zebra_count;
extern int line_flag;
extern int last_error;
extern int mid_speed;
extern int slow_speed;
extern int fast_speed;
extern int barrier_speed;
extern int dirc;
extern int far_num;
extern int close_num;
extern int curve_far_num;
extern int circle_save;
extern int circle_save_old;
extern int barrier_save;
extern int barrier_save_old;
extern int left_patch;
extern int right_patch;
extern uint8_t *PerImg_ip[240][320];
extern int16 encoder_right;
extern int16 encoder_left;
extern int line_chose;
extern float error_val;
extern float error_last;
extern int ramp_flag;
extern int circle_cold_time;
extern int ramp_flag_cold;
extern int expect_speed;
extern int circle_save_line[10000];
extern int barrier_save_line[10000];
extern int circle_flag_count;
extern int barrier_flag_count;
extern int barrier_cold_way;
extern int out_road;
extern int brick_save;
extern int barrier_hold_way;
extern int barrier_hold;
extern int barrier_hold_true;
extern int ramp_save;
extern int circle_sum_count;

int left_raw_lose = 0;
int right_raw_lose = 0;

int left_lenth = 0;
int right_lenth = 0;
int left_start = 0;
int right_start = 0;
int mid_lenth_left = 0;
int mid_lenth_right = 0;
int mid_lenth_raw_left = 0;
int mid_lenth_raw_right = 0;
int time_find_road = 0;
int test_bianliang;
int no_corner_count = 0;
int error_threshold = 55;
int mid_x = 0;
int mid_y = 0;
int left_straight = 0;
int right_straight = 0;

int16 imu_gyro_x_dif = 0;
int16 imu_gyro_x_old = 0;

int mid_lose = 0;
int left_width[240] = {0};
int right_width[240] = {0};
int road_width[240] = {0};
int barrier_start_num = 20;
int barrier_end_num = 5;
int mid_straight = 0;
int barrier_point[2] = {0};
float angle_barrier_save = 0;
ClassifyResult result;
#define LOCATION_Y 0
#define LOCATION_X 1

// 三条原始线，int 类型坐标
int left_line_raw[1000][2];
int right_line_raw[1000][2];
int mid_line_raw_from_left[1000][2];
int mid_line_raw_from_right[1000][2];

const float k_speed = 0.4f;
const float k_far = 0.6f;

// 定义方向向量：上、右、下、左（逆时针）
int dy[4] = {-1, 0, 1, 0};
int dx[4] = {0, 1, 0, -1};
int direction_l = 0; // 初始方向向上
int direction_r = 0; // 初始方向向上
int left_error_sum = 0;
int right_error_sum = 0;
int ttt = 0;

float gp = 0.0;
float gp_true = 0.0;
float angle_save = 0.0;
float angle_save_left = 0.0;
float angle_save_right = 0.0;
int speed_true = 0;
float error_raw = 0.0;
float expect_gyro = 0.0;

// 对左右边线原始数据进行滑动平均滤波
void filter_lines(int type, int strength)
{
    // 滤波强度必须是奇数且不小于3
    if (strength < 3)
        strength = 3;
    if (strength % 2 == 0)
        strength += 1;
    int half = strength / 2;

    // 指针变量
    int (*line_raw)[2];
    int *length_ptr;

    // 中间缓冲数组
    float tmp[900][2];

    // 选择对应的线
    if (type == 0)
    {
        line_raw = left_line_raw;
        length_ptr = &left_lenth;
    }
    else if (type == 1)
    {
        line_raw = right_line_raw;
        length_ptr = &right_lenth;
    }
    else
    {
        line_raw = mid_line_raw_from_left;
        length_ptr = &mid_lenth_left;
    }

    int num = *length_ptr; // 最远点下标
    for (int i = 0; i <= num; i++)
    {
        float sum_y = 0, sum_x = 0;
        int count = 0;
        for (int j = -half; j <= half; j++)
        {
            int idx = i + j;
            if (idx >= 0 && idx <= num)
            {
                sum_y += line_raw[idx][0];
                sum_x += line_raw[idx][1];
                count++;
            }
        }
        tmp[i][0] = sum_y / count;
        tmp[i][1] = sum_x / count;
    }

    // 将滤波结果写回原始数组
    for (int i = 0; i <= num; i++)
    {
        line_raw[i][0] = tmp[i][0];
        line_raw[i][1] = tmp[i][1];
    }
}

int find_edge_line()
{

    for (int i = 0; i < 239; i++)
    {

        left_width[i] = 0;
        right_width[i] = 319;
    }
    left_error_sum = 0;
    right_error_sum = 0;
    left_raw_lose = 0;
    right_raw_lose = 0;
    while (((left_line_raw[left_lenth][0] != right_line_raw[right_lenth][0]) || (left_line_raw[left_lenth][1] != right_line_raw[right_lenth][1])) &&
           (left_lenth < 900) && (right_lenth < 900) && (left_line_raw[left_lenth][0] != 0 || right_line_raw[right_lenth][0] != 0) && left_error_sum < 10 && right_error_sum < 10 && left_error_sum > -10 && right_error_sum > -10)
    {

        if (left_raw_lose <= right_raw_lose && left_line_raw[left_lenth][0] != 0 && left_line_raw[left_lenth][1] != 319)
        {
            line_grow_left();
            left_width[left_line_raw[left_lenth][0]] = left_line_raw[left_lenth][1];
        }
        else if (right_line_raw[right_lenth][0] != 0 && right_line_raw[right_lenth][1] != 0)
        {
            line_grow_right();
            right_width[right_line_raw[right_lenth][0]] = right_line_raw[right_lenth][1];
        }
        else if (left_line_raw[left_lenth][0] != 0 && left_line_raw[left_lenth][1] != 319)
        {

            line_grow_left();
            left_width[left_line_raw[left_lenth][0]] = left_line_raw[left_lenth][1];
        }
        else if (right_line_raw[right_lenth][0] != 0 && right_line_raw[right_lenth][1] != 0)
        {

            line_grow_right();
            right_width[right_line_raw[right_lenth][0]] = right_line_raw[right_lenth][1];
        }
        else
        {
            break;
        }
        // print_pixel(left_line_raw[left_lenth][0], left_line_raw[left_lenth][1], "blue");
        // print_pixel(right_line_raw[right_lenth][0], right_line_raw[right_lenth][1], "red");
        // msleep(10);
    }
    // system("clear");
    if ((left_lenth < 5 && right_lenth < 5) || (left_lenth == 900 && right_lenth == 900))
    {

        return -1;
    }

    return 0;
}

int chose_line_without_circle(int barrier_flag)
{

    switch (brick_save)
    {

    case 11:
        return 1;
    case 12:
        return 0;
    default:
        break;
    }

    if (left_patch > right_patch && left_lenth >= 5)
    {

        return 0;
    }
    else if (right_patch > left_patch && right_lenth >= 5)
    {

        return 1;
    }

    if (left_raw_lose > right_raw_lose + 3)
    {

        return 1;
    }
    else if (left_raw_lose + 3 < right_raw_lose)
    {

        return 0;
    }
    if (left_straight < right_straight)
    {

        return 0;
    }
    else if (left_straight > right_straight)
    {

        return 1;
    }

    if (((left_line[left_lenth][1] + right_line[right_lenth][1]) / 2) > 159)
    {

        return 0;
    }
    else
    {

        return 1;
    }
}

int chose_line(int circle_flag, int barrier_flag)
{   
    switch (circle_flag)
    {

    case 1:
    case 11:
    if(left_corner[0][0] || right_corner[0][0]){
        break;
    }
    
    case 22:
    case 31:
    case 42:
    case 52:
    case 61:
        return 1;
    case 2:
    case 12:
    if(left_corner[0][0] || right_corner[0][0]){
        break;
    }
    case 21:
    case 32:
    case 41:
    case 51:
    case 62:
        return 0;
    default:
        break;
    }

    switch (barrier_save)
    {

    case 0:
    case 1:
        return 0;
    case 2:
    case 3:
        return 1;
    default:
        break;
    }

    switch (brick_save)
    {

    case 11:
        return 1;
    case 12:
        return 0;
    default:
        break;
    }

    if (left_patch > right_patch && left_lenth >= 5)
    {

        return 0;
    }
    else if (right_patch > left_patch && right_lenth >= 5)
    {

        return 1;
    }

    if (left_raw_lose > right_raw_lose + 3)
    {

        return 1;
    }
    else if (left_raw_lose + 3 < right_raw_lose)
    {

        return 0;
    }
    if (left_straight < right_straight)
    {

        return 0;
    }
    else if (left_straight > right_straight)
    {

        return 1;
    }

    if (((left_line[left_lenth][1] + right_line[right_lenth][1]) / 2) > 159)
    {

        return 0;
    }
    else
    {

        return 1;
    }
}

int chose_line_width(int line)
{
    switch (brick_save)
    {

    case 1:

        if (line == 0)
        {
            return 70;
        }
        if (line == 1)
        {
            return 30;
        }
       break;

    case 2:
        if (line == 0)
        {
            return 30;
        }
        if (line == 1)
        {
            return 70;
        }
        break;
    default:
        break;
    }
    switch (barrier_save)
    {

    case 0:
    case 1:
        // if(barrier_hold_true / (barrier_hold_time * 1.0) < 3)

        if (angle_barrier_save < -25)
        {

            if (line == 0)
            {
                return -20;
            }
            if (line == 1)
            {
                return 120;
            }
        }
        else
        {
            if (line == 0)
            {
                return -20;
            }
            if (line == 1)
            {
                return 120;
            }
        }

        return 50;
    case 2:
    case 3:

        if (angle_barrier_save > 25)
        {

            if (line == 0)
            {
                return 120;
            }
            if (line == 1)
            {
                return -20;
            }
        }
        else
        {
            if (line == 0)
            {
                return 120;
            }
            if (line == 1)
            {
                return -20;
            }
        }

        return 50;
    default:
        return 50;
    }
}
void find_mid_line_from_left(int dir, int width)
{

    int (*line)[2];
    int lenth;
    int normal_dx_x = 0;
    int normal_dy_x = 0;
    int start;

    line = left_line;
    lenth = left_lenth;
    start = left_start;
    normal_dx_x = -1;
    normal_dy_x = 1;
    mid_line_raw_from_left[0][1] -= (50 - width);
    if (lenth >= 8)
    {
        for (int i = start + 4; i < lenth - 4; i++)
        {
            if (circle_save > 10 && circle_save < 50)
            {

                // if(line[i][1] < 23 || line[i][1] > 297){
                if (point_distance(line[i][1], line[i][0], line[i + 1][1], line[i + 1][0]) > 8 || line[i + 2][1] < 35 || line[i + 2][1] > 285)
                {
                    return;
                }
            }

            int num_minus_2 = i - 4; // 获取 i-4 元素的下标
            int num_plus_2 = i + 4;  // 获取 i+4 元素的下标

            // 获取 i-4 和 i+4 元素的坐标
            int x1 = line[num_minus_2][1];
            int y1 = line[num_minus_2][0];
            int x2 = line[num_plus_2][1];
            int y2 = line[num_plus_2][0];

            // 计算直线的斜率
            float dx = x2 - x1;
            float dy = y2 - y1;

            // 计算法线的方向（垂直于这条线）
            float normal_dx = dy * normal_dx_x;
            float normal_dy = dx * normal_dy_x;

            // 计算法线的长度，并将其单位化
            float length = sqrt(normal_dx * normal_dx + normal_dy * normal_dy);
            normal_dx /= length;
            normal_dy /= length;

            // 在法线方向上平移 50 个像素
            int mid_x = (x1 + x2) / 2 + normal_dx * width;
            int mid_y = (y1 + y2) / 2 + normal_dy * width;

            if (mid_x < 0 || mid_x > 319 || mid_y < 0)
            {
                return;
            }

            // 插入中线点
            if (mid_y <= 239)
            {

                mid_line_raw_from_left[mid_lenth_raw_left + 1][0] = mid_y;
                mid_line_raw_from_left[mid_lenth_raw_left + 1][1] = mid_x;
                mid_lenth_raw_left++;
            }
        }
    }
    if (lenth >= 4)
    {
        for (int i = lenth - 4; i <= lenth; i++)
        {
            if (lenth < 4)
            {

                break;
            }
            int x1 = line[lenth - 4][1];
            int y1 = line[lenth - 4][0];
            int x2 = line[lenth][1];
            int y2 = line[lenth][0];
            // 计算直线的斜率
            float dx = x2 - x1;
            float dy = y2 - y1;

            // 计算法线的方向（垂直于这条线）
            float normal_dx = dy * normal_dx_x;
            float normal_dy = dx * normal_dy_x;

            // 计算法线的长度，并将其单位化
            float length = sqrt(normal_dx * normal_dx + normal_dy * normal_dy);
            normal_dx /= length;
            normal_dy /= length;

            // 在法线方向上平移 50 个像素
            int mid_x = (x1 + x2) / 2 + normal_dx * width;
            int mid_y = (y1 + y2) / 2 + normal_dy * width;

            if (mid_x < 0 || mid_x > 319 || mid_y < 0)
            {
                return;
            }

            // 插入中线点
            if (mid_y <= 239)
            {

                mid_line_raw_from_left[mid_lenth_raw_left + 1][0] = mid_y;
                mid_line_raw_from_left[mid_lenth_raw_left + 1][1] = mid_x;
                mid_lenth_raw_left++;
            }
        }
    }
}

void find_mid_line_from_right(int dir, int width)
{

    int (*line)[2];
    int lenth;
    int normal_dx_x = 0;
    int normal_dy_x = 0;
    int start;

    line = right_line;
    lenth = right_lenth;
    start = right_start;
    normal_dx_x = 1;
    normal_dy_x = -1;
    mid_line_raw_from_right[0][1] += (50 - width);
    if (lenth >= 8)
    {
        for (int i = start + 4; i < lenth - 4; i++)
        {

            if (circle_save > 10 && circle_save < 50)
            {

                // if(line[i][1] < 23 || line[i][1] > 297){
                if (point_distance(line[i][1], line[i][0], line[i + 1][1], line[i + 1][0]) > 8 || line[i + 2][1] < 35 || line[i + 2][1] > 285)
                {
                    return;
                }
            }

            int num_minus_2 = i - 4; // 获取 i-4 元素的下标
            int num_plus_2 = i + 4;  // 获取 i+4 元素的下标

            // 获取 i-4 和 i+4 元素的坐标
            int x1 = line[num_minus_2][1];
            int y1 = line[num_minus_2][0];
            int x2 = line[num_plus_2][1];
            int y2 = line[num_plus_2][0];

            // 计算直线的斜率
            float dx = x2 - x1;
            float dy = y2 - y1;

            // 计算法线的方向（垂直于这条线）
            float normal_dx = dy * normal_dx_x;
            float normal_dy = dx * normal_dy_x;

            // 计算法线的长度，并将其单位化
            float length = sqrt(normal_dx * normal_dx + normal_dy * normal_dy);
            normal_dx /= length;
            normal_dy /= length;

            // 在法线方向上平移 50 个像素
            int mid_x = (x1 + x2) / 2 + normal_dx * width;
            int mid_y = (y1 + y2) / 2 + normal_dy * width;

            if (mid_x < 0 || mid_x > 319 || mid_y < 0)
            {
                return;
            }

            // 插入中线点
            if (mid_y <= 239)
            {

                mid_line_raw_from_right[mid_lenth_raw_right + 1][0] = mid_y;
                mid_line_raw_from_right[mid_lenth_raw_right + 1][1] = mid_x;
                mid_lenth_raw_right++;
            }
        }
    }
    if (lenth >= 4)
    {
        for (int i = lenth - 4; i <= lenth; i++)
        {

            int x1 = line[lenth - 4][1];
            int y1 = line[lenth - 4][0];
            int x2 = line[lenth][1];
            int y2 = line[lenth][0];
            // 计算直线的斜率
            float dx = x2 - x1;
            float dy = y2 - y1;

            // 计算法线的方向（垂直于这条线）
            float normal_dx = dy * normal_dx_x;
            float normal_dy = dx * normal_dy_x;

            // 计算法线的长度，并将其单位化
            float length = sqrt(normal_dx * normal_dx + normal_dy * normal_dy);
            normal_dx /= length;
            normal_dy /= length;

            // 在法线方向上平移 50 个像素
            int mid_x = (x1 + x2) / 2 + normal_dx * width;
            int mid_y = (y1 + y2) / 2 + normal_dy * width;

            if (mid_x < 0 || mid_x > 319 || mid_y < 0)
            {
                return;
            }

            // 插入中线点
            if (mid_y <= 239)
            {

                mid_line_raw_from_right[mid_lenth_raw_right + 1][0] = mid_y;
                mid_line_raw_from_right[mid_lenth_raw_right + 1][1] = mid_x;
                mid_lenth_raw_right++;
            }
        }
    }
}

float calculate_angle_float(int x1, int y1, int x2, int y2,
                            int x3, int y3, int x4, int y4)
{
    // 向量 P1P2
    int dx1 = x2 - x1;
    int dy1 = y2 - y1;

    // 向量 P3P4
    int dx2 = x4 - x3;
    int dy2 = y4 - y3;

    // 点积
    int dot_product = dx1 * dx2 + dy1 * dy2;

    // 模长
    double magnitude1 = sqrt(dx1 * dx1 + dy1 * dy1);
    double magnitude2 = sqrt(dx2 * dx2 + dy2 * dy2);

    // 防止除以0
    if (magnitude1 == 0 || magnitude2 == 0)
        return 0;

    // 夹角余弦
    double cos_theta = dot_product / (magnitude1 * magnitude2);
    if (cos_theta > 1)
        cos_theta = 1;
    if (cos_theta < -1)
        cos_theta = -1;

    // 夹角（角度）
    double angle_rad = acos(cos_theta);
    float angle_deg = (angle_rad * 180.0 / PI);

    // 方向：通过叉积判断
    int cross_product = dx1 * dy2 - dy1 * dx2;
    if (cross_product < 0)
    {
        angle_deg = -angle_deg;
    }

    /*/ / 计算补角（保留方向）
    int supplementary_angle = 180 - abs(angle_deg);
    if (angle_deg < 0)
        supplementary_angle = -supplementary_angle;

    return supplementary_angle;*/
    return angle_deg;
}

int calculate_angle(int x1, int y1, int x2, int y2,
                    int x3, int y3, int x4, int y4)
{
    // 向量 P1P2
    int dx1 = x2 - x1;
    int dy1 = y2 - y1;

    // 向量 P3P4
    int dx2 = x4 - x3;
    int dy2 = y4 - y3;

    // 点积
    int dot_product = dx1 * dx2 + dy1 * dy2;

    // 模长
    double magnitude1 = sqrt(dx1 * dx1 + dy1 * dy1);
    double magnitude2 = sqrt(dx2 * dx2 + dy2 * dy2);

    // 防止除以0
    if (magnitude1 == 0 || magnitude2 == 0)
        return 0;

    // 夹角余弦
    double cos_theta = dot_product / (magnitude1 * magnitude2);
    if (cos_theta > 1)
        cos_theta = 1;
    if (cos_theta < -1)
        cos_theta = -1;

    // 夹角（角度）
    double angle_rad = acos(cos_theta);
    int angle_deg = (int)(angle_rad * 180.0 / PI);

    // 方向：通过叉积判断
    int cross_product = dx1 * dy2 - dy1 * dx2;
    if (cross_product < 0)
    {
        angle_deg = -angle_deg;
    }

    /*/ / 计算补角（保留方向）
    int supplementary_angle = 180 - abs(angle_deg);
    if (angle_deg < 0)
        supplementary_angle = -supplementary_angle;

    return supplementary_angle;*/
    return angle_deg;
}

int left_start_save = 0;
int right_start_save = 319;
float find_curve_all()
{
    float error_val_left = find_curve_from_left();
    float error_val_right = find_curve_from_right();
    float error_val_mid = 0;
    float error_val = 0;
    /*  if (mid_lenth_left > 5 && mid_lenth_right > 5)
     {
         angle_save = (calculate_angle_float(159, 500, 159, 450,
                                             mid_line_from_left[0][1],
                                             mid_line_from_left[0][0],
                                             mid_line_from_left[mid_lenth_left][1],
                                             mid_line_from_left[mid_lenth_left][0]) +
                       calculate_angle_float(159, 500, 159, 450,
                                             mid_line_from_right[0][1],
                                             mid_line_from_right[0][0],
                                             mid_line_from_right[5][1],
                                             mid_line_from_right[5][0])) / 2.0;
     }

     else  */
    /*  if(abs(error_val_left - error_val_right) > 30){

         angle_save = 0;
     } */

    if (line_chose == 0 && mid_lenth_left > 5)
    {
        angle_save = angle_save_left;
    }
    else if (line_chose == 1 && mid_lenth_right > 5)
    {
        angle_save = angle_save_right;
    }
    else
    {

        angle_save = 0.0;
    }
    if (left_corner[0][0] > 0 && right_corner[0][0] > 0 && left_corner[1][1] == right_corner[1][1] && circle_save < 20)
    {
        circle_cold_time = 1000;
        angle_save = 0.0;
    }
    // if (!circle_save && left_corner[0][0] == 0 && right_corner[0][0] == 0 && barrier_save <= 0 && (ramp_save != 2))
    if (ramp_save != 2 && barrier_save < 0 && circle_save == 0 && brick_save == 0 && left_corner[0][0] == 0 && right_corner[0][0] == 0 && left_raw_lose == 0 && right_raw_lose == 0)
    {

        error_val = (error_val_left + error_val_right) / 2.0;
    }
    else if (ramp_save == 2)
    {
        // error_val_mid = find_curve_from_mid();
        error_val = error_val_mid;
    }
    else if (line_chose == 0)
    {
        error_val = error_val_left;
    }
    else
    {
        error_val = error_val_right;
    }
    if (out_road == -1 && ((barrier_save >= 0 && barrier_save < 4) || barrier_save == -4))
    {   
        int curve_out_road = (left_start_save + right_start_save) / 2 - 159;

        if (curve_out_road < 0)
        {
            error_val = -50;
        }
        else if (curve_out_road > 0)
        {
            error_val = 50;
        }
        // error_val = angle_save;
    }
    error_raw = error_val;
    error_val = (error_raw * ((speed_true) / 250.0));

    gp_true = gp;

    bool is_hold_low = (barrier_hold_true / (barrier_hold_way * 1.0) < 3);
    if ((barrier_save >= 0 && barrier_save < 6) && is_hold_low)
    {
        gp_true = gp_true * 1.25;
    }

    // gp_true = gp;

    // return (imu_gyro_z * 0.01 * gp_true - error_val);
    expect_gyro = error_val * gp_true * 100.0;
    // cout <<"expect_gyro: " << expect_gyro << endl;
    return error_val;
}

float find_curve_from_left()
{

    int lenth;
    int (*line)[2];
    int mid_num = 159;

    if (mid_lenth_left <= 5 && !circle_save)
    {

        switch (line_chose)
        {

        case 0:

            line = left_line;
            mid_num = 99;
            lenth = left_lenth;
            break;

        default:

            line = mid_line_from_left;
            lenth = mid_lenth_left;
            mid_num = 159;
        }
    }
    else
    {

        line = mid_line_from_left;
        lenth = mid_lenth_left;
        mid_num = 159;
    }

    int far = (int)(far_num * ((speed_true * 1.0) / mid_speed));
    if (far <= 5)
    {
        far = 5;
    }

    if (lenth < far)
    {

        far = lenth;
    }
    angle_save_left = (calculate_angle_float(159, 500, 159, 450,
                                             line[0][1],
                                             line[0][0],
                                             line[far][1],
                                             line[far][0]));
    int far_y = 239 - (far_num - 18) * 5;
    int sum_x1 = line[far][1];
    int sum_y1 = line[far][0];
    if (lenth <= 5)
    {

        return calculate_angle_float(mid_num, 500, mid_num, 450, mid_num, far_y, sum_x1, sum_y1);
    }
    /* for(int i = lenth; i >= 0; i--){
        sum_x1 = line[i][1];
        sum_y1 = line[i][0];
        if(point_dist(sum_x1, sum_y1, mid_num, far_y) <= (far * 5)){
            break;
        }
    } */
    int sum_x0 = line[far - 5][1];
    int sum_y0 = line[far - 5][0];

    // float angle1 = calculate_angle_float(mid_num, 500, mid_num, 450, sum_x0, sum_y0, sum_x1, sum_y1);
    float angle2 = calculate_angle_float(mid_num, 500, mid_num, 450, mid_num, far_y, sum_x1, sum_y1);
    // float angle3 = calculate_angle_float(mid_num, 500, mid_num, 450, mid_num, far_y, sum_x2, sum_y2);
    // float angle = angle1 * 0.1 + angle2 * 0.9;
    return angle2;
}

float find_curve_from_right()
{

    int lenth;
    int (*line)[2];
    int mid_num = 159;

    if (mid_lenth_right <= 5 && !circle_save)
    {

        switch (line_chose)
        {

        case 1:

            line = right_line;
            mid_num = 219;
            lenth = right_lenth;
            break;

        default:

            line = mid_line_from_right;
            lenth = mid_lenth_right;
            mid_num = 159;
        }
    }
    else
    {

        line = mid_line_from_right;
        lenth = mid_lenth_right;
        mid_num = 159;
    }

    int far = (int)(far_num * ((speed_true * 1.0) / mid_speed));
    if (far <= 5)
    {
        far = 5;
    }
    if (lenth < far)
    {

        far = lenth;
    }

    angle_save_right = (calculate_angle_float(159, 500, 159, 450,
                                              line[0][1],
                                              line[0][0],
                                              line[far][1],
                                              line[far][0]));
    int far_y = 239 - (far_num - 18) * 5;
    int sum_x1 = line[far][1];
    int sum_y1 = line[far][0];
    if (lenth <= 5)
    {

        return calculate_angle_float(mid_num, 500, mid_num, 450, mid_num, far_y, sum_x1, sum_y1);
    }
    /* for(int i = lenth; i >= 0; i--){
        sum_x1 = line[i][1];
        sum_y1 = line[i][0];
        if(point_dist(sum_x1, sum_y1, mid_num, far_y) <= (far * 5)){
            break;
        }
    } */
    int sum_x0 = line[far - 5][1];
    int sum_y0 = line[far - 5][0];
    // float angle1 = calculate_angle_float(mid_num, 500, mid_num, 450, sum_x0, sum_y0, sum_x1, sum_y1);
    float angle2 = calculate_angle_float(mid_num, 500, mid_num, 450, mid_num, far_y, sum_x1, sum_y1);
    // float angle3 = calculate_angle_float(mid_num, 500, mid_num, 450, mid_num, far_y, sum_x2, sum_y2);
    // float angle = angle1 * 0.1 + angle2 * 0.9;
    return angle2;
}
float find_curve_from_mid()
{
    int mid_num = 159;
    int sum_x1 = 0;
    int sum_y1 = 0;
    for (int i = 239; i >= 189; i--)
    {
        sum_x1 += (((right_width[i] + left_width[i]) / 2));
    }
    sum_x1 /= 50;
    sum_y1 = 200;
    return calculate_angle_float(mid_num, 500, mid_num, 450, mid_num, 239, sum_x1, sum_y1);
}

int get_expect_speed(int Error)
{
    mid_straight = 0;
    int lenth;
    int (*line)[2];

    if (line_chose == 0)
    {
        line = mid_line_from_left;
        lenth = mid_lenth_left;
    }
    else
    {
        line = mid_line_from_right;
        lenth = mid_lenth_right;
    }
    /* for (int i = 0; i <= lenth - 15; i++)
    {

        int num_last = i;      // 初始化前一个点的下标
        int num1 = i + 5;      // 获取当前点的下标
        int num2 = i + 10;     // 获取下一个点的下标
        int num_next = i + 15; // 获取下一个点的下标
        // 计算当前三个点的夹角
        int corner = calculate_angle(line[num_last][1], line[num_last][0],
                                     line[num1][1], line[num1][0],
                                     line[num2][1], line[num2][0],
                                     line[num_next][1], line[num_next][0]);

        // 小于-75度时，标记角落
        if (abs(corner) > 10)
        {

            mid_straight += abs(corner);
        }
    } */

    if (lenth >= 6) // 确保索引安全，防止 lenth - 5 变成负数
    {
        // 1. 获取末端点和前方参考点（使用 lenth-1 作为最末端数组索引）
        int end_idx = lenth - 1;
        double y = line[end_idx][0];
        double x = line[end_idx][1];
        double y2 = line[end_idx - 5][0];
        double x2 = line[end_idx - 5][1];
        double angle_line = calculate_angle_float(159, 500, 159, 450, x2, y2, x, y);

        // 2. 计算切线方向向量
        double dy = y - y2;
        double dx = x - x2;

        // 计算向量长度
        double distance = sqrt(dx * dx + dy * dy);

        // 防止两点重合导致除以 0 产生 NaN
        if (distance > 0.001)
        {
            // 归一化，得到单位步长（每次只移动1个像素距离）
            dy /= distance;
            dx /= distance;

            // 3. 设定延伸探测的起点（从末端往前迈出第一步）
            double current_y = y + dy;
            double current_x = x + dx;

            // 4. 沿切线方向步进探测
            // 只要不是黑色（!0），就一直沿切线往前走
            // 一旦超出图像边界，get_image_sum 自动返回 0，循环也会正常终止
            while (get_image_sum((int)(current_y + 0.5), (int)(current_x + 0.5)) == 255)
            {
                current_y += dy;
                current_x += dx;
            }

            // 5. 算出最终停下的 y 值（转换为你的 mid_straight 逻辑）
            mid_straight = ((int)(current_y + 0.5) * 15) + (int)(angle_line * 5);
        }
    }
    else
    {

        mid_straight = 9999;
    }

    // 1. 判断角度是否超出安全范围
    // bool is_sharp_angle = abs(angle_save) > 30;

    // 2. 判断障碍物状态是否在 0~3 之间
    bool is_valid_barrier = (barrier_save >= 0 && barrier_save <= 3);

    // 3. 判断障碍物保持率是否达标
    bool is_hold_low = (barrier_hold_true / (barrier_hold_way * 1.0) < 3);

    bool is_barrier_in = (((barrier_save == 0 || barrier_save == 1) && angle_barrier_save < -35) || ((barrier_save == 2 || barrier_save == 3) && angle_barrier_save > 35));

    // 4. 最终组合条件
    if (barrier_save == -3 && abs(angle_save) > 35)
    {
        return slow_speed;
    }
    if (is_valid_barrier && is_hold_low && is_barrier_in)
    {
        return slow_speed;
    }
    if (barrier_save == -3 || (is_valid_barrier && is_hold_low))
    {

        return barrier_speed;
    }
    if (is_valid_barrier || barrier_save == -3)
    {

        return barrier_speed;
    }
    if (ramp_save > 0 && ramp_save != 3)
    {

        return mid_speed;
    }
    if (circle_save > 10 && circle_save < 40)
    {
        // gp_true = gp;
        return mid_speed;
    }

    if (mid_straight > curve_far_num || mid_lenth_left < 30 || mid_lenth_right < 30)
    {
        // gp_true = gp;
        return mid_speed;
    }
    else
    {
        // gp_true = gp;
        return fast_speed;
    }
}

int point_distance(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2;
    int dy = y1 - y2;
    return (int)(sqrt(dx * dx + dy * dy));
}

int find_left_corner(int circle_flag)
{
    left_patch = 0;
    // 遍历左边线，直到结束点或越界
    for (int i = 0; i <= left_lenth - 12; i++)
    {

        int num_last = i;      // 初始化前一个点的下标
        int num1 = i + 4;      // 获取当前点的下标
        int num2 = i + 8;      // 获取下一个点的下标
        int num_next = i + 12; // 获取下一个点的下标
        // 计算当前三个点的夹角
        int corner = calculate_angle(left_line[num_last][1], left_line[num_last][0],
                                     left_line[num1][1], left_line[num1][0],
                                     left_line[num2][1], left_line[num2][0],
                                     left_line[num_next][1], left_line[num_next][0]);

        // 小于-75度时，标记角落
        if (abs(corner) > 10)
        {

            left_straight += abs(corner);
        }
        if (corner < -80 /* && (left_lose > 0 || right_lose > 0) */)
        {

            int d1 = point_distance(left_line[num_last][1], left_line[num_last][0], left_line[num1][1], left_line[num1][0]);
            int d2 = point_distance(left_line[num1][1], left_line[num1][0], left_line[num2][1], left_line[num2][0]);
            int d3 = point_distance(left_line[num2][1], left_line[num2][0], left_line[num_next][1], left_line[num_next][0]);
            // int d1 = left_line[num_last][1];
            // int d2 = left_line[num1][1];
            // int d3 = left_line[num2][1];
            // int d4 = left_line[num_next][1];
            if (d1 > 25 || d2 > 25 || d3 > 25) // 判断点之间的距离，防止在丢线处寻拐点
            {

                continue;
            }
            int angle_corner_down = calculate_angle(left_line[num_last][1], left_line[num_last][0] + 10,
                                                    left_line[num_last][1], left_line[num_last][0],
                                                    left_line[num_last][1], left_line[num_last][0],
                                                    left_line[num1][1], left_line[num1][0]);
            // cout << "left: " << (abs(angle_corner_down - angle_save) - 90) << endl;

            int angle_corner_up = calculate_angle(left_line[num2][1], left_line[num2][0] + 10,
                                                  left_line[num2][1], left_line[num2][0],
                                                  left_line[num2][1], left_line[num2][0],
                                                  left_line[num_next][1], left_line[num_next][0]);

            if (abs(angle_corner_down - angle_save) < 45)
            {

                // left_down_corner = num;
                // if (left_line[num1][0] >= 50)
                //{

                left_corner[0][0]++;
                left_corner[left_corner[0][0]][1] = 0;
                left_corner[left_corner[0][0]][0] = num1;
                left_patch += 2;
                //}
            }

            else if (abs(angle_corner_up - angle_save) < 45 && left_line[num_next][0] >= 0) // if (left_line[num2][1] <= 220)
            {

                // left_up_corner = num;
                // if (left_line[num_next][0] < left_line[num2][0] && left_line[num2][0] >= 50)
                //{

                left_corner[0][0]++;
                left_corner[left_corner[0][0]][1] = 1;
                left_corner[left_corner[0][0]][0] = num2;
                left_patch++;
                //}
            }
            else
            {

                left_corner[0][0]++;
                left_corner[left_corner[0][0]][1] = 2;
                left_corner[left_corner[0][0]][0] = num2;
                return 0;
            }
            /* else if (left_line[num1][0] >= 80)
            {
                left_corner[0][0]++;
                left_corner[left_corner[0][0]][1] = 0;
                left_corner[left_corner[0][0]][0] = num1;
                left_patch += 2;
            } */

            i += 10;
            continue;
        }
    }
    if ((circle_flag == 21) && left_corner[0][0] > 0 && left_corner[1][1] == 2)
    {

        left_corner[1][1] = 1;
        left_patch++;
    }
    if ((circle_flag == 32) && left_corner[0][0] > 0 && left_corner[1][1] != 0)
    {

        left_corner[1][1] = 0;
        left_patch++;
    }
    if ((circle_flag == 51) && left_corner[0][0] > 0 && left_corner[1][1] == 2)
    {

        left_corner[1][1] = 1;
        left_patch++;
    }

    return 0;
}

int find_right_corner(int circle_flag)
{
    right_patch = 0;
    // 遍历右边线，直到某个点或下一个点越界
    for (int i = 0; i <= right_lenth - 12; i++)
    {

        int num_last = i;      // 初始化前一个点的下标
        int num1 = i + 4;      // 获取当前点的下标
        int num2 = i + 8;      // 获取下一个点的下标
        int num_next = i + 12; // 获取下一个点的下标
        // 计算当前三个点的夹角
        int corner = calculate_angle(right_line[num_last][1], right_line[num_last][0],
                                     right_line[num1][1], right_line[num1][0],
                                     right_line[num2][1], right_line[num2][0],
                                     right_line[num_next][1], right_line[num_next][0]);

        if (abs(corner) > 10)
        {

            right_straight += abs(corner);
        }
        // 大于80度时，标记角落
        if (corner > 80 /*  && (right_lose > 0 || left_lose > 0) */)
        {

            int d1 = point_distance(right_line[num_last][1], right_line[num_last][0], right_line[num1][1], right_line[num1][0]);
            int d2 = point_distance(right_line[num1][1], right_line[num1][0], right_line[num2][1], right_line[num2][0]);
            int d3 = point_distance(right_line[num2][1], right_line[num2][0], right_line[num_next][1], right_line[num_next][0]);
            // int d1 = right_line[num_last][1];
            // int d2 = right_line[num1][1];
            // int d3 = right_line[num2][1];
            // int d4 = right_line[num_next][1];
            if (d1 > 25 || d2 > 25 || d3 > 25)
            {

                continue;
            }

            int angle_corner_down = calculate_angle(right_line[num_last][1], right_line[num_last][0] + 10,
                                                    right_line[num_last][1], right_line[num_last][0],
                                                    right_line[num_last][1], right_line[num_last][0],
                                                    right_line[num1][1], right_line[num1][0]);

            int angle_corner_up = calculate_angle(right_line[num2][1], right_line[num2][0] + 10,
                                                  right_line[num2][1], right_line[num2][0],
                                                  right_line[num2][1], right_line[num2][0],
                                                  right_line[num_next][1], right_line[num_next][0]);

            // cout << "right: " << (abs(angle_corner_down - angle_save) - 90) << endl;
            //  根据距离差异判断是右下角还是右上角
            if (abs(angle_corner_down - angle_save) < 45)
            {

                // right_down_corner = num;  // 右下角
                // if (right_line[num1][0] >= 50)
                //{
                right_corner[0][0]++;
                right_corner[right_corner[0][0]][1] = 0;
                right_corner[right_corner[0][0]][0] = num1;
                right_patch += 2;
                //}
            }

            else if (abs(angle_corner_up - angle_save) < 45 && right_line[num_next][0] >= 0) // if (right_line[num2][1] >= 100)
            {

                // right_up_corner = num;  // 右下角
                // if (right_line[num_next][0] < right_line[num2][0] && right_line[num2][0] >= 50)
                //{

                right_corner[0][0]++;
                right_corner[right_corner[0][0]][1] = 1;
                right_corner[right_corner[0][0]][0] = num2;
                right_patch++;
                //}
            }
            else
            {

                right_corner[0][0]++;
                right_corner[right_corner[0][0]][1] = 2;
                right_corner[right_corner[0][0]][0] = num2;
                return 0;
            }
            /* else if (right_line[num1][0] >= 80)
            {
                right_corner[0][0]++;
                right_corner[right_corner[0][0]][1] = 0;
                right_corner[right_corner[0][0]][0] = num1;
                right_patch += 2;
            } */

            // 更新位置并继续遍历
            i += 10;
            continue;
        }
    }
    if ((circle_flag == 22) && right_corner[0][0] > 0 && right_corner[1][1] == 2)
    {

        right_corner[1][1] = 1;
        right_patch++;
    }
    if ((circle_flag == 31) && right_corner[0][0] > 0 && right_corner[1][1] != 0)
    {

        right_corner[1][1] = 0;
        right_patch++;
    }
    if ((circle_flag == 52) && right_corner[0][0] > 0 && right_corner[1][1] == 2)
    {

        right_corner[1][1] = 1;
        right_patch++;
    }
    return 0;
}

int find_zebra()
{
    constexpr int scan_row_begin = 200;
    constexpr int scan_row_end = 193;
    constexpr int min_transitions_per_row = 11;

    int matching_rows = 0;

    for (int row = scan_row_begin; row >= scan_row_end; --row)
    {
        // 边线可能来自补线或默认值，扫描前统一限制到图像有效范围。
        const int start = left_width[row];
        const int end = right_width[row];
        if (end - start < 2)
        {
            continue;
        }

        int transitions = 0;
        int prev = get_image_sum(row, start);
        int curr = get_image_sum(row, start + 1);

        // 从第三个像素开始，避免重复读取起点并保证三个采样点连续。
        for (int col = start + 2; col <= end; ++col)
        {
            const int next = get_image_sum(row, col);

            // 要求跳变前至少有两个连续同值像素，以抑制孤立噪点。
            if (prev == curr && curr != next)
            {
                ++transitions;
            }

            prev = curr;
            curr = next;
        }

        if (transitions >= min_transitions_per_row)
        {
            ++matching_rows;
        }
    }

    return matching_rows;
}

void patch_line(int circle_flag)
{
    left_start = right_start = 0;
    // 1. 合并前置拦截条件（原逻辑中两个 if 的合并）
    if (circle_save % 10 == 1 && circle_save > 20 && circle_save < 60)
    {

        for (int i = 0; i <= left_lenth; i++)
        {

            if (left_line[i][1] == 0)
            {

                left_lenth = i;
                break;
            }
        }
    }
    if (circle_save % 10 == 2 && circle_save > 20 && circle_save < 60)
    {

        for (int i = 0; i <= right_lenth; i++)
        {

            if (right_line[i][1] == 319)
            {

                right_lenth = i;
                break;
            }
        }
    }

    if (circle_save > 20 && circle_save < 40)
    {
        return;
    }

    // 初始化起点

    // ==================== 2. 左边控制逻辑 ====================
    int left_case = left_corner[0][0];
    if (left_case == 0)
    {
        for (int i = 0; i <= left_lenth; i++)
        {

            if (left_line[i][1] == 0)
            {

                left_lenth = i;
                break;
            }
        }
    }
    else if (left_case == 1)
    {
        // 合并了原来对 0 和 2 的相同处理
        if (left_corner[1][1] == 0 || left_corner[1][1] == 2)
        {
            left_lenth = left_corner[1][0];
        }
        else if (left_corner[1][1] == 1)
        {
            left_start = left_corner[1][0];
        }
    }
    else if (left_case >= 2 && left_case <= 5)
    {
        if (left_corner[2][1] == 0)
        {
            left_lenth = (left_corner[1][1] == 0) ? left_corner[1][0] : left_corner[2][0];
        }
        if (left_corner[1][1] == 1)
        {
            left_start = left_corner[1][0];
        }
        if (left_corner[1][1] == 0 && left_corner[2][1] == 2)
        {
            left_lenth = left_corner[1][0];
        }
        if (left_corner[2][1] == 1)
        {
            left_start = left_corner[2][0];
        }
        if (left_corner[3][1] == 1 && left_case >= 3 && left_corner[2][1] != 2 && left_corner[1][1] != 2)
        {
            left_start = left_corner[3][0];
        }
    }

    // ==================== 3. 右边控制逻辑（已修复 Bug） ====================
    int right_case = right_corner[0][0];
    if (right_case == 0)
    {
        for (int i = 0; i <= right_lenth; i++)
        {

            if (right_line[i][1] == 319)
            {

                right_lenth = i;
                break;
            }
        }
    }
    else if (right_case == 1)
    {
        // 【已修复】这里原本误写成了 left_corner，现已修正为真正的右路逻辑
        if (right_corner[1][1] == 0 || right_corner[1][1] == 2)
        {
            right_lenth = right_corner[1][0];
        }
        else if (right_corner[1][1] == 1)
        {
            right_start = right_corner[1][0];
        }
    }
    else if (right_case >= 2 && right_case <= 5)
    {
        if (right_corner[2][1] == 0)
        {
            right_lenth = (right_corner[1][1] == 0) ? right_corner[1][0] : right_corner[2][0];
        }
        if (right_corner[1][1] == 1)
        {
            right_start = right_corner[1][0];
        }
        if (right_corner[1][1] == 0 && right_corner[2][1] == 2)
        {
            right_lenth = right_corner[1][0];
        }
        if (right_corner[2][1] == 1)
        {
            right_start = right_corner[2][0];
        }
        if (right_corner[3][1] == 1 && right_case >= 3 && right_corner[2][1] != 2 && right_corner[1][1] != 2)
        {
            right_start = right_corner[3][0];
        }
    }
}

int find_circle(int circle_flag)
{

    switch (circle_flag)
    {

    case 0:
    {

        if (left_corner[0][0] > 0 && right_corner[0][0] > 0)
        {
            return 0;
        }

        if (left_corner[0][0] > 0 && left_corner[1][1] == 0 && right_straight < 100 && (left_raw_lose + right_raw_lose > 5))
        {

            no_corner_count = 0;
            return 1;
        }
        else if (right_corner[0][0] > 0 && right_corner[1][1] == 0 && left_straight < 100 && (left_raw_lose + right_raw_lose > 5))
        {

            no_corner_count = 0;
            return 2;
        }
        else if (right_straight < 100 && right_raw_lose == 0 && left_corner[0][0] > 0 && left_corner[1][1] == 1)
        {
            if (no_corner_count > 2)
            {
                no_corner_count = 0;
                return 21;
            }
            else
            {
                no_corner_count++;
                return 0;
            }
        }
        else if (left_straight < 100 && left_raw_lose == 0 && right_corner[0][0] > 0 && right_corner[1][1] == 1)
        {
            if (no_corner_count > 2)
            {
                no_corner_count = 0;
                return 22;
            }
            else
            {
                no_corner_count++;
                return 0;
            }
        }
        else
        {
            no_corner_count = 0;
            return 0;
        }
    }

    case 1:
    {

        if (right_corner[0][0] == 0 && left_corner[0][0] == 0)
        {

            no_corner_count = 0;
            return 11;
        }
        else if ((right_lenth < 25 || right_corner[0][0] > 0 || right_straight > 200) && left_corner[0][0] == 0 && left_corner[1][1] != 0)
        {
            circle_save_old = 0;
            return 0;
        }
        else
        {

            return 1;
        }
    }
    case 2:
    {

        if (left_corner[0][0] == 0 && right_corner[0][0] == 0)
        {

            no_corner_count = 0;
            return 12;
        }

        else if ((left_lenth < 25 || left_corner[0][0] > 0 || left_straight > 200) && (right_corner[0][0] == 0 || right_corner[1][1] != 0))
        {
            circle_save_old = 0;
            return 0;
        }
        else
        {

            return 2;
        }
    }

    case 11:
    {

        if (left_corner[0][0] > 0 && right_corner[0][0] == 0 && left_corner[1][1] == 1)
        {

            if (no_corner_count >= 2)
            {

                no_corner_count = 0;
                return 21;
            }
            no_corner_count++;
            return 11;
        }
        else if (right_corner[0][0] > 0 && right_corner[1][1] == 0 && (left_raw_lose == 0))
        {

            no_corner_count = 0;
            return 2;
        }
        else
        {

            return 11;
        }
    }
    case 12:
    {

        if (right_corner[0][0] > 0 && left_corner[0][0] == 0 && right_corner[1][1] == 1)
        {

            if (no_corner_count >= 2)
            {

                no_corner_count = 0;
                return 22;
            }
            no_corner_count++;
            return 12;
        }
        else if (left_corner[0][0] > 0 && left_corner[1][1] == 0 && (right_raw_lose == 0))
        {

            no_corner_count = 0;
            return 1;
        }
        else
        {

            return 12;
        }
    }

    case 21:
    {

        if (left_corner[0][0] == 0 && right_corner[0][0] == 0)
        {

            if (no_corner_count > 5)
            {

                return 31;
            }
            else
            {

                no_corner_count++;
                return 21;
            }
        }
        else
        {
            no_corner_count = 0;
            return 21;
        }
    }

    case 22:
    {
        if (left_corner[0][0] == 0 && right_corner[0][0] == 0)
        {

            if (no_corner_count > 5)
            {

                return 32;
            }
            else
            {

                no_corner_count++;
                return 22;
            }
        }
        else
        {
            no_corner_count = 0;
            return 22;
        }
    }

    case 31:
    {
        if (left_corner[0][0] == 0 && right_corner[0][0] > 0 && right_corner[1][1] == 0)
        {

            return 41;
        }
        if (left_corner[0][0] > 0 && right_straight < 100 && barrier_save < 0)
        {

            return 0;
        }

        else
        {

            return 31;
        }
    }
    case 32:
    {
        if (right_corner[0][0] == 0 && left_corner[0][0] > 0 && left_corner[1][1] == 0)
        {

            return 42;
        }

        if (right_corner[0][0] > 0 && left_straight < 100 && barrier_save < 0)
        {

            return 0;
        }
        else
        {

            return 32;
        }
    }

    case 41:
    {

        /* if ((left_corner[0][0] > 0 && left_corner[1][1] == 1 && right_corner[0][0] == 0 && right_straight < 100) || (right_straight < 100 && right_lenth > 40 && abs(angle_save) < 30))
        {

            return 51;
        } */
        if (left_corner[0][0] > 0 && left_corner[1][1] == 1 && right_corner[0][0] == 0)
        {
            no_corner_count = 0;
            return 61;
        }
        else
        {

            return 41;
        }
    }

    case 42:
    {
        /* if ((right_corner[0][0] > 0 && right_corner[1][1] == 1 && left_corner[0][0] == 0 && left_straight < 100) || (left_straight < 100 && left_lenth > 40 && abs(angle_save) < 30))
        {
            return 52;
        } */
        if (right_corner[0][0] > 0 && right_corner[1][1] == 1 && left_corner[0][0] == 0)
        {
            no_corner_count = 0;
            return 62;
        }
        else
        {

            return 42;
        }
    }

    case 51:
    {
        if (left_corner[0][0] > 0 && left_corner[1][1] == 1 && right_corner[0][0] == 0)
        {
            no_corner_count = 0;
            return 61;
        }
        else
        {

            return 51;
        }
    }
    case 52:
    {
        if (right_corner[0][0] > 0 && right_corner[1][1] == 1 && left_corner[0][0] == 0)
        {
            no_corner_count = 0;
            return 62;
        }
        else
        {

            return 52;
        }
    }
    case 61:
    {

        if (left_corner[0][0] > 0 && left_corner[1][1] == 1 && right_corner[0][0] == 0)
        {
            no_corner_count --;
            return 61;
        }
        else
        {   
            if(no_corner_count > 5){
                circle_sum_count++;
                return 0;
            }
            else{
                no_corner_count++;
                return 61;
            }
            
            return 0;
        }
    }
    case 62:
    {

        if (right_corner[0][0] > 0 && right_corner[1][1] == 1 && left_corner[0][0] == 0)
        {
            no_corner_count--;
            return 62;
        }
        else
        {
            if (no_corner_count > 5)
            {
                circle_sum_count++;
                return 0;
            }
            else
            {
                no_corner_count++;
                return 62;
            }
            return 0;
        }
    }

    default:
        return 0;
    }
}

// 左边线巡线函数
void line_grow_left()
{

    int y = left_line_raw[left_lenth][0];
    int x = left_line_raw[left_lenth][1];
    // 计算左方、左前方坐标
    int left_dir = (direction_l + 3) % 4; // 左方
    int left_y = y + dy[left_dir];
    int left_x = x + dx[left_dir];
    int left_front_y = y + dy[left_dir] + dy[direction_l]; // 左前方
    int left_front_x = x + dx[left_dir] + dx[direction_l];
    if (get_image_sum(left_y, left_x) != 255 && get_image_sum(left_front_y, left_front_x) != 255)
    {
        // 左和左前都黑，直走
        int front_y = y + dy[direction_l];
        int front_x = x + dx[direction_l];
        if (get_image_sum(front_y, front_x) == 255)
        {

            y = front_y;
            x = front_x;

            // add_line_pixel(0, left_line_x[0][3], 1, y, x);
            left_line_raw[left_lenth + 1][0] = y;
            left_line_raw[left_lenth + 1][1] = x;
            // if(x == 0){
            if (get_image_sum(left_y, left_x) == 125)
            {

                left_raw_lose++;
            }
            left_lenth++;
        }
        else
        {
            direction_l = (direction_l + 1) % 4;
            left_error_sum++;
        }
    }
    else if (get_image_sum(left_y, left_x) != 255 && get_image_sum(left_front_y, left_front_x) == 255)
    {
        // 左黑 左前白，左转走
        y = left_front_y;
        x = left_front_x;
        direction_l = (direction_l + 3) % 4;
        left_error_sum--;
        // add_line_pixel(0, left_line_x[0][3], 1, y, x);
        left_line_raw[left_lenth + 1][0] = y;
        left_line_raw[left_lenth + 1][1] = x;
        // if(x == 0){
        if (get_image_sum(left_y, left_x) == 125)
        {

            left_raw_lose++;
        }
        left_lenth++;
    }
    else
    {
        direction_l = (direction_l + 1) % 4;
        left_error_sum++;
    }
}

// 右边线巡线函数（对齐左边线逻辑）
void line_grow_right()
{
    int y = right_line_raw[right_lenth][0];
    int x = right_line_raw[right_lenth][1];

    // 计算右方、右前方坐标
    int right_dir = (direction_r + 1) % 4; // 右方
    int right_y = y + dy[right_dir];
    int right_x = x + dx[right_dir];
    int right_front_y = y + dy[right_dir] + dy[direction_r]; // 右前方
    int right_front_x = x + dx[right_dir] + dx[direction_r];

    if (get_image_sum(right_y, right_x) != 255 && get_image_sum(right_front_y, right_front_x) != 255)
    {
        // 右和右前都黑，直走
        int front_y = y + dy[direction_r];
        int front_x = x + dx[direction_r];
        if (get_image_sum(front_y, front_x) == 255)
        {
            y = front_y;
            x = front_x;

            // add_line_pixel(1, right_line_x[0][3], 1, y, x);
            right_line_raw[right_lenth + 1][0] = y;
            right_line_raw[right_lenth + 1][1] = x;
            // if(x == 319){
            if (get_image_sum(right_y, right_x) == 125)
            {

                right_raw_lose++;
            }
            right_lenth++;
        }
        else
        {
            direction_r = (direction_r + 3) % 4; // 转向（逆时针）
            right_error_sum++;
        }
    }
    else if (get_image_sum(right_y, right_x) != 255 && get_image_sum(right_front_y, right_front_x) == 255)
    {
        // 右黑 右前白，右转走
        y = right_front_y;
        x = right_front_x;
        direction_r = (direction_r + 1) % 4; // 顺时针右转
        right_error_sum--;

        // add_line_pixel(1, right_line_x[0][3], 1, y, x);
        right_line_raw[right_lenth + 1][0] = y;
        right_line_raw[right_lenth + 1][1] = x;
        // if(x == 319){
        if (get_image_sum(right_y, right_x) == 125)
        {

            right_raw_lose++;
        }
        right_lenth++;
    }
    else
    {
        // 其他情况，尝试转向
        direction_r = (direction_r + 3) % 4; // 转向（逆时针）
        right_error_sum++;
    }
}

float point_dist(float y1, float x1, float y2, float x2)
{
    float dy = y2 - y1, dx = x2 - x1;
    return sqrtf(dy * dy + dx * dx);
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

// 图像坐标系 (0,0) 在左上角，y轴向下
// 第一条线：k > 0 (从左上到右下)
#define P1_X 0
#define P1_Y 151
#define P2_X 91
#define P2_Y 239

// 第二条线：k < 0 (从左下到右上)
#define P3_X 234
#define P3_Y 239
#define P4_X 319
#define P4_Y 162

/**
 * 判断点 (px, py) 是否在 P1P2 右侧且在 P3P4 左侧 (图像坐标系)
 */
int isPointInRegion(int px, int py)
{
    // 叉乘公式不变
    // CrossProduct = (x2 - x1) * (py - y1) - (y2 - y1) * (px - x1)

    long long cp1 = (long long)(P2_X - P1_X) * (py - P1_Y) -
                    (long long)(P2_Y - P1_Y) * (px - P1_X);

    long long cp2 = (long long)(P4_X - P3_X) * (py - P3_Y) -
                    (long long)(P4_Y - P3_Y) * (px - P3_X);

    // 在图像坐标系 (y轴向下) 中：
    // cp > 0 : 点在向量右侧
    // cp < 0 : 点在向量左侧
    // cp = 0 : 点在直线上

    bool at_right_of_line1 = (cp1 > 0);
    bool at_left_of_line2 = (cp2 < 0);

    return (at_right_of_line1 && at_left_of_line2) ? 1 : 0;
}

void resample_track_line(int type, float spacing)
{
    int (*src_int)[2];  // 原始整型线
    int (*dst_int)[2];  // 输出整型线
    int *length_ptr;    // 指向长度变量
    int *lenth_ptr_raw; // 指向原始长度变量

    // 选择源、目标数组和对应长度变量
    if (type == 0)
    {
        src_int = left_line_raw;
        dst_int = left_line;
        length_ptr = &left_lenth;
        lenth_ptr_raw = &left_lenth;
    }
    else if (type == 1)
    {
        src_int = right_line_raw;
        dst_int = right_line;
        length_ptr = &right_lenth;
        lenth_ptr_raw = &right_lenth;
    }
    else if (type == 2)
    {
        src_int = mid_line_raw_from_left;
        dst_int = mid_line_from_left;
        length_ptr = &mid_lenth_left;
        lenth_ptr_raw = &mid_lenth_raw_left;
    }
    else if (type == 3)
    {
        src_int = mid_line_raw_from_right;
        dst_int = mid_line_from_right;
        length_ptr = &mid_lenth_right;
        lenth_ptr_raw = &mid_lenth_raw_right;
    }
    else
    {
        return; // 无效类型，直接返回
    }

    // 获取原始点数量，防止越界
    int num = *lenth_ptr_raw + 1; // 最远点下标 + 1 为点的数量
    if (num > 900)
        num = 900; // 防止越界，最多处理 900 个点

    // ===== 【核心修复 1】: 处理极端少点情况，防止后续流程引发除 0 崩溃 =====
    if (num <= 1)
    {
        if (num == 1)
        {
            dst_int[0][0] = src_int[0][0];
            dst_int[0][1] = src_int[0][1];
            *length_ptr = 0; // 下标为 0
        }
        else
        {
            *length_ptr = -1; // 空线
        }
        return;
    }

    // 将整型坐标转换为浮点，便于计算
    float src[900][2];
    for (int i = 0; i < num; i++)
    {
        src[i][0] = (float)src_int[i][0]; // y 坐标
        src[i][1] = (float)src_int[i][1]; // x 坐标
    }

    // 计算每段的长度以及总长度
    float seg_len[900];
    float total_len = 0;
    for (int i = 0; i < num - 1; i++)
    {
        seg_len[i] = point_dist(src[i][0], src[i][1], src[i + 1][0], src[i + 1][1]);
        total_len += seg_len[i];
    }

    // 计算新的采样点数量（上限900）
    int new_num_est = (int)(total_len / spacing) + 1;
    if (new_num_est > 900)
        new_num_est = 900;

    float acc_len = 0;
    int seg_index = 0;
    int valid_num = 0; // 有效点数量

    // 第一个点直接复制，但也要判断是否有效
    int y0 = (int)(src[0][0] + 0.5f);
    int x0 = (int)(src[0][1] + 0.5f);

    dst_int[valid_num][0] = y0;
    dst_int[valid_num][1] = x0;
    valid_num++;

    // 从第二个点开始重采样
    for (int i = 1; i < new_num_est; i++)
    {
        float d = i * spacing;

        // 找到采样点对应的线段
        while (seg_index < num - 1 && acc_len + seg_len[seg_index] < d)
        {
            acc_len += seg_len[seg_index];
            seg_index++;
        }

        // ===== 【安全保护】: 兜底逻辑，防止 seg_index 越界导致访问 seg_len 异常 =====
        if (seg_index >= num - 1)
            break;

        // ===== 【核心修复 2】: 极端情况下如果线段长度为 0，防止除法产生 NaN 或 INF =====
        if (seg_len[seg_index] < 0.00001f)
            continue;

        float remain = d - acc_len;
        float t = remain / seg_len[seg_index];

        // 插值计算新的采样点
        float y = lerp(src[seg_index][0], src[seg_index + 1][0], t);
        float x = lerp(src[seg_index][1], src[seg_index + 1][1], t);

        int iy = (int)(y + 0.5f);
        int ix = (int)(x + 0.5f);

        // ===== 【安全保护】: 增加 valid_num > 0 的判断，防止 dst_int[valid_num - 1] 越界（当第一个点没有写入时） =====
        if (type == 2 || (ix != 0 && ix != 319 && iy != 0) ||
            (valid_num > 0 && (ix == 0 || ix == 319) && dst_int[valid_num - 1][1] != 0 && dst_int[valid_num - 1][1] != 319))
        {
            dst_int[valid_num][0] = iy;
            dst_int[valid_num][1] = ix;
            valid_num++;
            if (valid_num >= 900)
                break; // 防止越界
        }
    }

    // 更新长度变量为有效点数量
    // ===== 【核心修复 3】: 如果 valid_num 为 0 则是异常情况，将其限制最小为 0，防止给长度赋值 -1 =====
    *length_ptr = (valid_num > 0) ? (valid_num - 1) : 0;
}

#define ROI_X_MIN 0
#define ROI_Y_MIN 0
#define ROI_X_MAX 319
#define ROI_Y_MAX 239

extern uint8_t *PerImg_Color_ip_1[RESULT_ROW][RESULT_COL];
extern uint8_t *PerImg_Color_ip[RESULT_ROW][RESULT_COL];

HSV hsv;
#define H_THRESHOLD 20.0f // 红色 H ≤ 20
#define S_THRESHOLD 0.5f  // 排除白/黑
#define V_THRESHOLD 0.2f  // 排除黑

int find_barrier(int type)
{
    int (*src_int)[2];
    int *length_ptr;
    int card_flag = 0;
    int barrier_far = 999;

    if (type == 0)
    {
        // src_int = mid_line_from_left;
        // length_ptr = &mid_lenth_left;
        src_int = mid_line_from_left;
        length_ptr = &mid_lenth_left;
    }
    else if (type == 1)
    {
        // src_int = mid_line_from_right;
        // length_ptr = &mid_lenth_right;
        src_int = mid_line_from_right;
        length_ptr = &mid_lenth_right;
    }
    else
    {
        return -1;
    }

    int i = 1;
    if (barrier_save < 0)
    {
        // 从第1个点开始，因为需要前一个点计算方向
        for (i = 1; i < *length_ptr; i++)
        {
            int y = src_int[i][0];
            int x = src_int[i][1];
            hsv = yuyv_to_hsv(y, x);
            bool is_point_in_threshold = (hsv.h < H_THRESHOLD || hsv.h > 360 - H_THRESHOLD) && hsv.s > S_THRESHOLD && hsv.v > V_THRESHOLD;

            // if(hsv.h > 20){
            if (!is_point_in_threshold)
            {
                continue;
            }
            // if (get_image_sum(y, x) == 255)
            //     continue;

            // --- 2. 沿垂线两边计数 ---
            int barrier_count_l = 0;
            int barrier_count_r = 0;
            int barrier_count_u = 0;
            int barrier_count_d = 0;
            // j从1开始，不重复计算中心点；如果需要包含中心点，可另加逻辑
            /* for (int j = 0; j < 5; j++)
            {
                if ((get_image_sum(y + j, x) == 0) && (y + j <= 239))
                {
                    barrier_count_d++;
                }

                if ((get_image_sum(y - j, x) == 0) && (y - j >= 0))
                {
                    barrier_count_u++;
                }
                if ((get_image_sum(y, x + j) == 0) && (y + j <= 239))
                {
                    barrier_count_r++;
                }

                if ((get_image_sum(y, x - j) == 0) && (y - j >= 0))
                {
                    barrier_count_l++;
                }
            } */

            /* for (int j = 0; j < 5; j++)
            {
                hsv = yuyv_to_hsv(y + j, x);
                if (hsv.v != 0.0f && hsv.h < H_THRESHOLD && hsv.s > S_THRESHOLD && hsv.v > V_THRESHOLD && (y + j <= 239))
                    barrier_count_d++;

                hsv = yuyv_to_hsv(y - j, x);
                if (hsv.v != 0.0f && hsv.h < H_THRESHOLD && hsv.s > S_THRESHOLD && hsv.v > V_THRESHOLD && (y - j >= 0))
                    barrier_count_u++;

                hsv = yuyv_to_hsv(y, x + j);
                if (hsv.v != 0.0f && hsv.h < H_THRESHOLD && hsv.s > S_THRESHOLD && hsv.v > V_THRESHOLD && (x + j <= 319))
                    barrier_count_r++;

                hsv = yuyv_to_hsv(y, x - j);
                if (hsv.v != 0.0f && hsv.h < H_THRESHOLD && hsv.s > S_THRESHOLD && hsv.v > V_THRESHOLD && (x - j >= 0))
                    barrier_count_l++;
            } */
            for (int j = 0; j < 5; j++)
            {
                if ((get_image_sum(y + j, x) == 0) && (y + j <= 239))
                {
                    barrier_count_d++;
                }

                if ((get_image_sum(y - j, x) == 0) && (y - j >= 0))
                {
                    barrier_count_u++;
                }
                if ((get_image_sum(y, x + j) == 0) && (x + j <= 319))
                {
                    barrier_count_r++;
                }

                if ((get_image_sum(y, x - j) == 0) && (x - j >= 0))
                {
                    barrier_count_l++;
                }
            }

            // --- 3. 判定逻辑 ---
            // system("clear");
            if ((barrier_count_l >= 5 || barrier_count_r >= 5) && (barrier_count_u >= 5 || barrier_count_d >= 5))
            {
                /* system("clear");
                for (int i = 0; i < 240; i++)
                {
                    print_pixel(i, left_width[i], "blue");
                    print_pixel(i, right_width[i], "red");
                }

                print_point_and_neighbors(y, x);
                msleep(300); */

                int line_width_on_barrier = right_width[y] - left_width[y];
                int is_barrier_in_image = 0;
                int is_barrier_mid_in_line = 0;

                if (y > 0 && y < 240 && x > 10 && x < 310)
                {
                    is_barrier_in_image = 1;
                }
                if (x > left_width[y] && x < right_width[y])
                {
                    is_barrier_mid_in_line = 1;
                }
                int prev = get_image_sum(y, 79);
                int curr = get_image_sum(y, 80);

                // system("clear");
                if ((line_width_on_barrier > 80 || (left_width[y] > 0 && left_width[y] < 15) || (right_width[y] < 319 && right_width[y] > 304)) && is_barrier_in_image == 1 && is_barrier_mid_in_line == 1)
                {
                    if (i + 5 < *length_ptr)
                    {

                        barrier_point[0] = src_int[i + 4][0] + src_int[i + 5][0];
                        barrier_point[1] = src_int[i + 4][1] + src_int[i + 5][1];

                        card_flag = 1;
                        angle_barrier_save = calculate_angle_float(159, 500, 159, 400, src_int[i][1], src_int[i][0], src_int[i + 5][1], src_int[i + 5][0]);
                        cut_image(barrier_point[0], barrier_point[1], 75);
                        barrier_far = point_distance(barrier_point[1] / 2, barrier_point[0] / 2, 159, 289) - 50;
                        break;
                    }
                }
                /* else
                {
                    cout << "line_width_on_barrier: " << line_width_on_barrier << endl;
                    cout << "left_width[y]: " << left_width[y] << endl;
                    cout << "right_width[y]: " << right_width[y] << endl;
                    cout << "is_barrier_in_image: " << is_barrier_in_image << endl;
                    cout << "is_barrier_mid_in_line: " << is_barrier_mid_in_line << " " << x << " " << y << endl;
                    cout << "is_barrier_without_zebra: " << is_barrier_without_zebra << endl;
                    for (int i = 0; i < 240; i++)
                    {
                        print_pixel(i, left_width[i], "blue");
                        print_pixel(i, right_width[i], "red");
                    }
                    print_point_and_neighbors(y,x);
                    msleep(200);
                } */
            }
        }
    }
    /* system("clear");
    if (card_flag)
    {

        int bx_x = barrier_point[1] / 2;
        int by_y = barrier_point[0] / 2;
        cout << "card_flag: " << card_flag << endl;
        cout << by_y << " " << bx_x << endl;
        hsv = yuyv_to_hsv(src_int[i][0], src_int[i][1]);
        cout << "h: " << hsv.h << endl;
        cout << "s: " << hsv.s << endl;
        cout << "v: " << hsv.v << endl;
        for (int i = 0; i < 240; i++)
        {
            print_pixel(i, left_width[i], "blue");
            print_pixel(i, right_width[i], "red");
        }
        print_point_and_neighbors(by_y, bx_x);
        msleep(300);
    } */
    switch (barrier_save)
    {

    case -1:
    {

        int bx = barrier_point[1] / 2;
        int by = barrier_point[0] / 2;

        // 定义剔除区域（左下和右下直角三角形）
        bool in_left_corner = 0;

        bool in_right_corner = 0;

        /* if (card_flag)
        {

            system("clear");
            cout << "card_flag: " << card_flag << endl;
            cout << "i: " << i << " " << barrier_start_num << endl;
            cout << "in_left_corner: " << in_left_corner << endl;
            cout << "in_right_corner: " << in_right_corner << endl;
            cout << by << " " << bx << endl;
            for (int i = 0; i < 240; i++)
            {

                for (int j = 0; j < 320; j++)
                {

                    if ((i - j) > 195)
                    {

                        print_pixel(i, j, "blue");
                    }
                    else if ((i + j) > 515)
                    {

                        print_pixel(i, j, "red");
                    }
                }
            }
            print_point_and_neighbors(by, bx);
        } */
        if (by - bx > 195)
        {

            in_left_corner = 1;
        }
        if (by + bx > 515)
        {

            in_right_corner = 1;
        }
        if (card_flag && barrier_far < barrier_start_num && !in_left_corner && !in_right_corner)
        {
            return -3;
        }
        /* else
        {
            system("clear");
            if (card_flag)
            {

                system("clear");
                cout << "card_flag: " << card_flag << endl;
                cout << "i: " << i << " " << barrier_start_num << endl;
                cout << "in_left_corner: " << in_left_corner << endl;
                cout << "in_right_corner: " << in_right_corner << endl;
                cout << by << " " << bx << endl;
                for(int i = 0; i < 240; i++){

                    for(int j = 0; j < 320; j++){

                        if((i - j) > 195){

                            print_pixel(i, j, "blue");
                        }
                        else if((i + j) > 515){

                            print_pixel(i, j, "red");
                        }
                    }
                }
                print_point_and_neighbors(by, bx);
            }
        } */
        return -1;
    }
    case -2:
    {
        if (card_flag && barrier_far < barrier_end_num)
        {

            result = model_identify(square);
            barrier_save_line[barrier_flag_count] = result.sort;
            barrier_flag_count++;
            return result.sort;
            // return 2;
        }
        else if (card_flag)
        {

            return -3;
        }
        return -2;
    }
    case -3:
    {

        if (card_flag && barrier_far < barrier_end_num)
        {

            // snprintf(path, sizeof(path), "/home/root/image/image%d.png", mycount);
            // mycount++;
            // imwrite(path, square);
            result = model_identify(square);
            barrier_save_line[barrier_flag_count] = result.sort;
            barrier_flag_count++;
            return result.sort;
            // return 2;
        }
        else if (card_flag)
        {

            return -3;
        }
        else
        {

            return -2;
        }
    }
    case -4:
    {
        if (card_flag && barrier_far < barrier_end_num)
        {

            // snprintf(path, sizeof(path), "/home/root/image/image%d.png", mycount);
            // mycount++;
            // imwrite(path, square);
            result = model_identify(square);
            barrier_save_line[barrier_flag_count] = result.sort;
            barrier_flag_count++;
            return result.sort;
            // return 2;
        }
        else if (card_flag)
        {

            return -3;
        }
        else
        {

            return -4;
        }
    }
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return barrier_save;
    default:
        return -1;
    }
}

int find_barrier_accurate(int type)
{
    int (*src_int)[2];
    int *length_ptr;
    int card_flag = 0;
    int barrier_far = 999;

    if (type == 0)
    {
        src_int = mid_line_from_left;
        length_ptr = &mid_lenth_left;
    }
    else if (type == 1)
    {
        src_int = mid_line_from_right;
        length_ptr = &mid_lenth_right;
    }
    else
    {
        return -1;
    }
    if (barrier_save < 0)
    {
        // 先沿中线寻找红色点，作为红色矩形的粗定位种子。
        for (int i = 1; i < *length_ptr; ++i)
        {
            const int seed_y = src_int[i][0];
            const int seed_x = src_int[i][1];
            HSV seed_hsv = yuyv_to_hsv(seed_y, seed_x);
            const bool seed_is_red =
                (seed_hsv.h < H_THRESHOLD || seed_hsv.h > 360.0f - H_THRESHOLD) &&
                seed_hsv.s > S_THRESHOLD && seed_hsv.v > V_THRESHOLD;
            if (!seed_is_red)
            {
                continue;
            }

            // 当前候选红区精定位失败时，跳过其中连续的中线红点，
            // 再由外层循环寻找下一个红区；成功时仍立即结束本次调用。
            auto skip_current_red_region = [&]()
            {
                while (i + 1 < *length_ptr)
                {
                    const int next_y = src_int[i + 1][0];
                    const int next_x = src_int[i + 1][1];
                    HSV next_hsv = yuyv_to_hsv(next_y, next_x);
                    const bool next_is_red =
                        (next_hsv.h < H_THRESHOLD ||
                         next_hsv.h > 360.0f - H_THRESHOLD) &&
                        next_hsv.s > S_THRESHOLD &&
                        next_hsv.v > V_THRESHOLD;
                    if (!next_is_red)
                    {
                        break;
                    }
                    ++i;
                }
            };

            // 中线索引增大的方向就是“较远”方向 t，n 与中线垂直且近似平行于红条长边。
            const int near_index = std::max(0, i - 3);
            const int far_index = std::min(*length_ptr, i + 3);
            Point2f far_direction(
                (float)(src_int[far_index][1] - src_int[near_index][1]),
                (float)(src_int[far_index][0] - src_int[near_index][0]));
            float far_direction_length = std::sqrt(
                far_direction.x * far_direction.x + far_direction.y * far_direction.y);
            if (far_direction_length < 0.001f)
            {
                skip_current_red_region();
                continue;
            }
            far_direction.x /= far_direction_length;
            far_direction.y /= far_direction_length;
            const Point2f lateral_direction(-far_direction.y, far_direction.x);

            // 红条约 26x11 像素，在粗定位点周围取足够大的 ROI。
            const int roi_radius = 40;
            const int roi_x0 = std::max(0, seed_x - roi_radius);
            const int roi_y0 = std::max(0, seed_y - roi_radius);
            const int roi_x1 = std::min(RESULT_COL - 1, seed_x + roi_radius);
            const int roi_y1 = std::min(RESULT_ROW - 1, seed_y + roi_radius);
            const int roi_width = roi_x1 - roi_x0 + 1;
            const int roi_height = roi_y1 - roi_y0 + 1;
            if (roi_width <= 0 || roi_height <= 0)
            {
                skip_current_red_region();
                continue;
            }

            // 三态缓存：0=未计算，1=非红色，2=红色。只转换扫描真正访问的像素，
            // 同一像素被相邻倾斜扫描线访问时直接复用结果。
            Mat red_mask = Mat::zeros(roi_height, roi_width, CV_8UC1);
            auto mask_is_red = [&](const Point2f &point) -> bool
            {
                const int image_x = cvRound(point.x);
                const int image_y = cvRound(point.y);
                const int local_x = image_x - roi_x0;
                const int local_y = image_y - roi_y0;
                if (local_x < 0 || local_x >= roi_width ||
                    local_y < 0 || local_y >= roi_height)
                {
                    return false;
                }

                uchar &cached = red_mask.ptr<uchar>(local_y)[local_x];
                if (cached == 0)
                {
                    HSV pixel_hsv = yuyv_to_hsv(image_y, image_x);
                    const bool is_red =
                        (pixel_hsv.h < H_THRESHOLD ||
                         pixel_hsv.h > 360.0f - H_THRESHOLD) &&
                        pixel_hsv.s > S_THRESHOLD &&
                        pixel_hsv.v > V_THRESHOLD;
                    cached = is_red ? 2 : 1;
                }
                return cached == 2;
            };
            // 按需缓存中红色值为2；如需显示，先转为0/255后再调用此测试函数。
            // print_image(red_mask, 0, 0);

            // 对每个横向位置，从近到远扫描。只取首次连续进入红色区域的位置，
            // 因而矩形远侧即使与其他红色色块粘连，也不会改变近侧边缘候选点。
            vector<Point2f> edge_candidates;
            const int lateral_scan_half = 35;
            const int longitudinal_scan_half = 35;
            const int required_red_run = 3;
            const int edge_depth_tolerance = 16;
            edge_candidates.reserve(2 * lateral_scan_half + 1);
            const Point2f seed_point((float)seed_x, (float)seed_y);

            // 种子点位于红条内，先在中心扫描线上完整搜索一次近侧边缘深度。
            // 长边与中线夹角误差限制为20度，其余扫描线只需搜索该深度附近。
            int center_edge_depth = longitudinal_scan_half + 1;
            Point2f center_scan_point =
                seed_point - far_direction * (float)longitudinal_scan_half;
            int center_red_run = 0;
            for (int longitudinal = -longitudinal_scan_half;
                 longitudinal <= longitudinal_scan_half;
                 ++longitudinal, center_scan_point += far_direction)
            {
                if (mask_is_red(center_scan_point))
                {
                    ++center_red_run;
                    if (center_red_run >= required_red_run)
                    {
                        center_edge_depth = longitudinal - required_red_run + 1;
                        break;
                    }
                }
                else
                {
                    center_red_run = 0;
                }
            }
            if (center_edge_depth > longitudinal_scan_half)
            {
                skip_current_red_region();
                continue;
            }

            const int scan_depth_start = std::max(
                -longitudinal_scan_half,
                center_edge_depth - edge_depth_tolerance);
            const int scan_depth_end = std::min(
                longitudinal_scan_half,
                center_edge_depth + edge_depth_tolerance + required_red_run - 1);

            for (int lateral = -lateral_scan_half; lateral <= lateral_scan_half; ++lateral)
            {
                Point2f scan_point =
                    seed_point + lateral_direction * (float)lateral +
                    far_direction * (float)scan_depth_start;
                int red_run = 0;
                for (int longitudinal = scan_depth_start;
                     longitudinal <= scan_depth_end;
                     ++longitudinal, scan_point += far_direction)
                {
                    if (mask_is_red(scan_point))
                    {
                        ++red_run;
                        if (red_run >= required_red_run)
                        {
                            edge_candidates.push_back(
                                scan_point - far_direction * (float)(required_red_run - 1));
                            break;
                        }
                    }
                    else
                    {
                        red_run = 0;
                    }
                }
            }
            if (edge_candidates.size() < 10)
            {
                skip_current_red_region();
                continue;
            }

            // RANSAC：寻找与中线近似垂直、具有足够连续跨度的最近直线。
            // 候选点已经按 lateral 顺序排列，使用少量固定间隔产生确定性假设线。
            // 每条假设线只统计内点，不再创建临时 vector；最终仅收集一次最佳内点。
            vector<int> best_inliers;
            best_inliers.reserve(edge_candidates.size());
            float best_near_projection = std::numeric_limits<float>::max();
            int best_inlier_count = 0;
            Point2f best_line_origin(0.0f, 0.0f);
            Point2f best_line_direction(0.0f, 0.0f);
            bool best_line_found = false;
            const float max_inlier_distance = 1.75f;
            const float max_parallel_dot = std::sin(20.0f * PI / 180.0f);
            const int sample_offsets[] = {8, 12, 16, 20, 24, 28, 32};
            const int sample_offset_count =
                sizeof(sample_offsets) / sizeof(sample_offsets[0]);
            const int candidate_count = (int)edge_candidates.size();

            for (int a = 0; a < candidate_count; ++a)
            {
                for (int offset_index = 0;
                     offset_index < sample_offset_count; ++offset_index)
                {
                    const int b = a + sample_offsets[offset_index];
                    if (b >= candidate_count)
                    {
                        break;
                    }

                    Point2f line_direction = edge_candidates[b] - edge_candidates[a];
                    const float line_length = std::sqrt(
                        line_direction.x * line_direction.x +
                        line_direction.y * line_direction.y);
                    if (line_length < 8.0f)
                    {
                        continue;
                    }
                    line_direction.x /= line_length;
                    line_direction.y /= line_length;
                    if (std::abs(line_direction.x * far_direction.x +
                                 line_direction.y * far_direction.y) > max_parallel_dot)
                    {
                        continue;
                    }

                    int inlier_count = 0;
                    Point2f inlier_sum(0.0f, 0.0f);
                    float min_along = std::numeric_limits<float>::max();
                    float max_along = -std::numeric_limits<float>::max();
                    for (int candidate_index = 0;
                         candidate_index < candidate_count;
                         ++candidate_index)
                    {
                        const Point2f delta =
                            edge_candidates[candidate_index] - edge_candidates[a];
                        const float distance = std::abs(
                            delta.x * line_direction.y -
                            delta.y * line_direction.x);
                        if (distance <= max_inlier_distance)
                        {
                            ++inlier_count;
                            inlier_sum += edge_candidates[candidate_index];
                            const float along = delta.x * line_direction.x +
                                                delta.y * line_direction.y;
                            min_along = std::min(min_along, along);
                            max_along = std::max(max_along, along);
                        }
                    }

                    const float span = max_along - min_along;
                    if (inlier_count < 10 || span < 12.0f)
                    {
                        continue;
                    }

                    const Point2f inlier_mean =
                        inlier_sum * (1.0f / (float)inlier_count);
                    const float near_projection =
                        inlier_mean.x * far_direction.x +
                        inlier_mean.y * far_direction.y;

                    const bool is_nearer =
                        near_projection < best_near_projection - 0.5f;
                    const bool same_depth_but_better =
                        std::abs(near_projection - best_near_projection) <= 0.5f &&
                        inlier_count > best_inlier_count;
                    if (is_nearer || same_depth_but_better)
                    {
                        best_near_projection = near_projection;
                        best_inlier_count = inlier_count;
                        best_line_origin = edge_candidates[a];
                        best_line_direction = line_direction;
                        best_line_found = true;
                    }
                }
            }

            if (best_line_found)
            {
                for (int candidate_index = 0;
                     candidate_index < candidate_count;
                     ++candidate_index)
                {
                    const Point2f delta =
                        edge_candidates[candidate_index] - best_line_origin;
                    const float distance = std::abs(
                        delta.x * best_line_direction.y -
                        delta.y * best_line_direction.x);
                    if (distance <= max_inlier_distance)
                    {
                        best_inliers.push_back(candidate_index);
                    }
                }
            }
            if (best_inliers.size() < 10)
            {
                skip_current_red_region();
                continue;
            }

            vector<Point2f> near_edge_points;
            near_edge_points.reserve(best_inliers.size());
            for (int inlier_index : best_inliers)
            {
                near_edge_points.push_back(edge_candidates[inlier_index]);
            }

            Vec4f fitted_line;
            fitLine(near_edge_points, fitted_line, DIST_L2, 0, 0.01, 0.01);
            Point2f edge_direction(fitted_line[0], fitted_line[1]);
            Point2f line_point(fitted_line[2], fitted_line[3]);
            if (edge_direction.x < 0.0f ||
                (std::abs(edge_direction.x) < 0.001f && edge_direction.y < 0.0f))
            {
                edge_direction.x = -edge_direction.x;
                edge_direction.y = -edge_direction.y;
            }

            float min_along = std::numeric_limits<float>::max();
            float max_along = -std::numeric_limits<float>::max();
            for (const Point2f &point : near_edge_points)
            {
                const Point2f delta = point - line_point;
                const float along = delta.x * edge_direction.x +
                                    delta.y * edge_direction.y;
                min_along = std::min(min_along, along);
                max_along = std::max(max_along, along);
            }
            const float fitted_span = max_along - min_along;
            if (fitted_span < 12.0f)
            {
                skip_current_red_region();
                continue;
            }

            const Point2f near_edge_mid =
                line_point + edge_direction * ((min_along + max_along) * 0.5f);
            Point2f edge_normal(-edge_direction.y, edge_direction.x);
            if (edge_normal.x * far_direction.x + edge_normal.y * far_direction.y < 0.0f)
            {
                edge_normal.x = -edge_normal.x;
                edge_normal.y = -edge_normal.y;
            }

            const Point2f card_center = near_edge_mid + edge_normal * 24.0f;
            const int x = cvRound(card_center.x);
            const int y = cvRound(card_center.y);
            if (x < 0 || x >= RESULT_COL || y < 0 || y >= RESULT_ROW)
            {
                skip_current_red_region();
                continue;
            }

            // 卡片朝向取“近侧长边 -> 卡片中心”的有向法线。
            // 以屏幕竖直向上 (0, -1) 为 0 度；图像坐标 y 向下，
            // atan2(x, -y) 的正角表示顺时针旋转，范围为 [-180, +180]。
            angle_barrier_save =
                std::atan2(edge_normal.x, -edge_normal.y) * 180.0f / PI;
            barrier_point[0] = y;
            barrier_point[1] = x;
            cut_image(y * 2, x * 2, 75);
            barrier_far = point_distance(x, y, 159, 289) - 50;
            card_flag = 1;
            // print_image(square, 0, 244);
            break;
        }
    }
    bool card_in_image = 0;
    if (get_image_sum(barrier_point[0], barrier_point[1] - 12) != 125 && get_image_sum(barrier_point[0], barrier_point[1] + 12) != 125)
    {
        card_in_image = 1;
    }
    switch (barrier_save)
    {

    case -1:
    {

        if (card_flag && card_in_image && barrier_far < barrier_start_num)
        {
            return -3;
        }
        /* else
        {
            system("clear");
            if (card_flag)
            {

                system("clear");
                cout << "card_flag: " << card_flag << endl;
                cout << "i: " << i << " " << barrier_start_num << endl;
                cout << "in_left_corner: " << in_left_corner << endl;
                cout << "in_right_corner: " << in_right_corner << endl;
                cout << by << " " << bx << endl;
                for(int i = 0; i < 240; i++){

                    for(int j = 0; j < 320; j++){

                        if((i - j) > 195){

                            print_pixel(i, j, "blue");
                        }
                        else if((i + j) > 515){

                            print_pixel(i, j, "red");
                        }
                    }
                }
                print_point_and_neighbors(by, bx);
            }
        } */
        return -1;
    }
    case -2:
    {
        if (card_flag && barrier_far < barrier_end_num && card_in_image)
        {

            result = model_identify(square);
            barrier_save_line[barrier_flag_count] = result.sort;
            barrier_flag_count++;
            return result.sort;
            // return 2;
        }
        else if (card_flag)
        {

            return -3;
        }
        return -2;
    }
    case -3:
    {

        if (card_flag && barrier_far < barrier_end_num && card_in_image)
        {

            // snprintf(path, sizeof(path), "/home/root/image/image%d.png", mycount);
            // mycount++;
            // imwrite(path, square);
            result = model_identify(square);
            barrier_save_line[barrier_flag_count] = result.sort;
            barrier_flag_count++;
            return result.sort;
            // return 2;
        }
        else if (card_flag)
        {

            return -3;
        }
        else
        {

            return -2;
        }
    }
    case -4:
    {
        if (card_flag && barrier_far < barrier_end_num && card_in_image)
        {

            // snprintf(path, sizeof(path), "/home/root/image/image%d.png", mycount);
            // mycount++;
            // imwrite(path, square);
            result = model_identify(square);
            barrier_save_line[barrier_flag_count] = result.sort;
            barrier_flag_count++;
            return result.sort;
            // return 2;
        }
        else if (card_flag)
        {

            return -3;
        }
        else
        {

            return -4;
        }
    }
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return barrier_save;
    default:
        return -1;
    }
}

int find_brick()
{
    int max_barrier_length = 0; // 记录最长区间的长度
    int current_length = 0;     // 记录当前连续区间的长度

    int max_start_idx = -1; // 最长区间的起始下标
    int max_end_idx = -1;   // 最长区间的结束下标
    int brick_flag = 0;

    for (int i = 0; i < 240; i++)
    {
        if (right_width[i] - left_width[i] < 80)
        {
            current_length++; // 满足条件，当前连续长度加 1

            // 如果当前长度超过了历史最长长度，更新最长区间的信息
            if (current_length > max_barrier_length)
            {
                max_barrier_length = current_length;
                max_end_idx = i;                        // 当前 i 就是最长区间的结束下标
                max_start_idx = i - current_length + 1; // 通过结束下标和长度推算起始下标
            }
        }
        else
        {
            current_length = 0; // 条件不满足，连续区间中断，清零重新计数
        }
    }
    int point_lose = 0;
    int point_lose_d = 0;
    int point_lose_u = 0;
    if (max_barrier_length > 60)
    {

        for (int i = max_start_idx; i <= max_end_idx; i++)
        {
            if (left_width[i] == 0 || right_width[i] == 319)
            {
                point_lose++;
            }
        }
        int start = max_start_idx >= 40 ? max_start_idx - 40 : 0;
        int end = max_end_idx <= 199 ? max_end_idx + 40 : 239;
        for (int i = start; i < max_start_idx; i++)
        {

            int width = right_width[i] - left_width[i];
            if (width < 80 || width > 150 || left_width[i] == 0 || right_width[i] == 319)
            {

                point_lose_d++;
            }
        }
        for (int i = max_end_idx + 1; i < end; i++)
        {

            int width = right_width[i] - left_width[i];
            if (width < 80 || width > 150 || left_width[i] == 0 || right_width[i] == 319)
            {

                point_lose_u++;
            }
        }
        if (point_lose == 0 && ((point_lose_d == 0 && max_start_idx - start >= 20) || (point_lose_u == 0 && end - max_end_idx >= 20)))
        {

            brick_flag = 1;
        }
    }
    /* system("clear");
    cout << "max_barrier_length: " << max_barrier_length << endl;
    cout << "left_straight: " << left_straight << endl;
    cout << "right_straight: " << right_straight << endl;
    cout << "left_raw_lose: " << left_raw_lose << endl;
    cout << "right_raw_lose: " << right_raw_lose << endl;
    cout << "brick_flag: " << brick_flag << endl; */
    /* switch(brick_save){
        case 0:
            if (brick_flag == 1)
            {
                if (point_lose == 0 && (right_straight > (left_straight + 360)))
                {
                    //cout << "brick_save return 1" << endl;
                    return 2;
                }
                else if (point_lose == 0 && (left_straight > (right_straight + 360)))
                {
                    //cout << "brick_save return 2" << endl;
                    return 1;
                }
                else
                {
                    //cout << "brick_save return 0" << endl;
                    return 0;
                }
            }
            //cout << "brick_save return 0" << endl;
            return 0;
        case 1:
        case 2:
            //cout << "brick_save return " << brick_save << endl;
            return brick_save;
        default:
            return 0;

    } */
    if (brick_flag == 1)
    {
        if (point_lose == 0 && (right_straight > (left_straight + 360)))
        {
            // cout << "brick_save return 1" << endl;
            return 2;
        }
        else if (point_lose == 0 && (left_straight > (right_straight + 360)))
        {
            // cout << "brick_save return 2" << endl;
            return 1;
        }
        else
        {
            // cout << "brick_save return 0" << endl;
            return 0;
        }
    }
    // cout << "brick_save return 0" << endl;
    return 0;
}
void line_init()
{

    int max_len = 0;
    int curr_len = 0;
    int curr_start = 0;
    int start_x_left = -1;
    int start_x_right = -1;
    int start_num = 0;
    int end_num = 0;
    if(barrier_save >= 0 || barrier_save == -4){

        
        if (out_road == -1)
        {

            int curve_out_road = (left_start_save + right_start_save) / 2 - 159;

            if (curve_out_road <= 0)
            {
                start_num = 0;
                end_num = 159;
            }
            else
            {
                start_num = 159;
                end_num = 319;
            }
        }
        else
        {
            start_num = left_start_save;
            end_num = right_start_save;
        }
    }
    else
    {
        start_num = 0;
        end_num = 319;
    }

    for (int x = start_num; x < end_num; ++x)
    {
        if (get_image_sum(239, x) == 255)
        { // 白色像素
            if (curr_len == 0)
            {
                curr_start = x;
            }
            curr_len++;
        }
        else
        {
            if (curr_len > max_len)
            {
                max_len = curr_len;
                start_x_left = curr_start;
                start_x_right = x - 1;

                max_len = curr_len;
                start_x_left = curr_start;
                start_x_right = x - 1;
            }
            curr_len = 0;
        }
    }
    // 处理末尾是白色的情况
    if (curr_len > max_len)
    {
        max_len = curr_len;
        start_x_left = curr_start;
        start_x_right = end_num;
    }
    if (barrier_save < 0 && max_len < 90)
    {
        int start_x_left_term1 = start_num;
        int start_x_right_term1 = start_num;
        int start_x_left_term2 = end_num;
        int start_x_right_term2 = end_num;
        while (get_image_sum(239, start_x_left_term1) != 255 && start_x_left_term1 < 159)
        {
            start_x_left_term1++;
        }

        start_x_right_term1 = start_x_left_term1;
        while (get_image_sum(239, start_x_right_term1 + 1) == 255 && start_x_right_term1 < 319)
        {
            start_x_right_term1++;
        }

        while (get_image_sum(239, start_x_right_term2) != 255 && start_x_right_term2 >= 159)
        {
            start_x_right_term2--;
        }

        start_x_left_term2 = start_x_right_term2;
        while (get_image_sum(239, start_x_left_term2 - 1) == 255 && start_x_left_term2 > 0)
        {
            start_x_left_term2--;
        }
        int term_len = 0;
        term_len = start_x_right_term2 - start_x_left_term1;
        if (term_len > 50 && term_len < 150)
        {

            start_x_left = start_x_left_term1;
            start_x_right = start_x_right_term2;
        }
        term_len = start_x_right_term1 - start_x_left_term1;
        if (term_len > 50 && term_len < 150 && angle_save < 0)
        {
            start_x_left = start_x_left_term1;
            start_x_right = start_x_right_term1;
        }
        term_len = start_x_right_term2 - start_x_left_term2;
        if (term_len > 50 && term_len < 150 && angle_save >= 0)
        {

            start_x_left = start_x_left_term2;
            start_x_right = start_x_right_term2;
        }
    }
    while (get_image_sum(239, start_x_left - 1) == 255 && start_x_left > 0)
    {
        start_x_left--;
    }
    while (get_image_sum(239, start_x_right + 1) == 255 && start_x_right < 319)
    {
        start_x_right++;
    }
    if(start_x_left != -1){

        left_start_save = start_x_left;
    }
    if(start_x_right != -1){

        right_start_save = start_x_right;
    }
    left_line_raw[0][0] = 239;          // 行坐标
    left_line_raw[0][1] = start_x_left; // 列坐标
    left_width[239] = start_x_left;
    left_lenth = 0;
    left_start = 0;

    right_line_raw[0][0] = 239;
    right_line_raw[0][1] = start_x_right;
    right_width[239] = start_x_right;
    right_lenth = 0;
    right_start = 0;

    mid_line_raw_from_left[0][0] = 239;
    mid_line_raw_from_right[0][0] = 239;
    mid_line_raw_from_left[0][1] = (start_x_left + start_x_right) / 2;
    mid_line_raw_from_right[0][1] = (start_x_left + start_x_right) / 2;
    mid_lenth_left = 0;
    mid_lenth_right = 0;
    mid_lenth_raw_left = 0;
    mid_lenth_raw_right = 0;
    left_corner[0][0] = 0;
    right_corner[0][0] = 0;
}

int imu_x_count = 0;
extern int ramp_cold_time;
int find_ramp()
{

    int width_0 = right_width[239] - left_width[239];
    int width_1 = right_width[120] - left_width[120];
    int width_2 = right_width[160] - left_width[160];
    int mid_1 = abs((right_width[120] + left_width[120]) / 2 - 159);
    int mid_2 = abs((right_width[160] + left_width[160]) / 2 - 159);
    int imu_threshold = (speed_true / 200) * 2000;
    if (imu_threshold < 2000)
    {

        imu_threshold = 2000;
    }
    /* system("clear");
    cout << "width_1: " << width_1 << endl;
    cout << "width_2: " << width_2 << endl;
    cout << "lose: " << left_raw_lose << " " << right_raw_lose << endl;
    cout << (right_width[120] + left_width[120]) / 2 << endl;
    cout << (right_width[160] + left_width[160]) / 2 << endl;
    cout << right_width[239] - left_width[239] << endl; */
    imu_gyro_x = imu_get_raw(imu_file_path[GYRO_X_RAW]);
    imu_gyro_x_dif = imu_gyro_x - imu_gyro_x_old;
    imu_gyro_x_old = imu_gyro_x;
    switch (ramp_save)
    {
    case 0:
        if (width_1 > 120 && width_2 > 120 && left_raw_lose == 0 && right_raw_lose == 0 && mid_1 < 50 && mid_2 < 50 && left_lenth > 40 && right_lenth > 40 && barrier_save == -1)
        {

            return 1;
        }
        return 0;
    case 1:
        if (imu_gyro_x_dif > imu_threshold)
        {

            return 2;
        }
        return 1;
    case 2:
        if (abs(width_0 - 100) < 10 && abs(width_2 - 100) < 10 && abs(width_1 - 100) < 10)
        {

            return 3;
        }
        return 2;
    case 3:
        return 3;
    default:
        ramp_cold_time = 100;
        return 0;
    }
    return 0;
}