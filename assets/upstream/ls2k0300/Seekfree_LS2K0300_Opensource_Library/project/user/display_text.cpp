#include "display_text.hpp"
#include "display_image.hpp"

#define DISPLAY_TEXT_X                0
#define DISPLAY_TEXT_Y                124
#define DISPLAY_TEXT_STEP             16
#define DISPLAY_TEXT_COL2_X           120
#define DISPLAY_TEXT_AREA_Y           120
#define DISPLAY_TEXT_AREA_W           240

static void display_text_draw_separator(void)
{
    uint16_t x = 0;
    zf_device_ips200 *ips200 = display_image_get_device();

    if(NULL == ips200)
    {
        return;
    }

    for(x = 0; x < DISPLAY_TEXT_AREA_W; x++)
    {
        ips200->draw_point(x, DISPLAY_TEXT_AREA_Y, RGB565_WHITE);
    }
}

void display_text_init(void)
{
    display_text_draw_separator();
}

// void display_text_show(const VisionResult *vision, const chassis_info_t *chassis)
// {
//     uint16_t row0 = DISPLAY_TEXT_Y;
//     uint16_t row1 = DISPLAY_TEXT_Y + DISPLAY_TEXT_STEP;
//     uint16_t row2 = DISPLAY_TEXT_Y + DISPLAY_TEXT_STEP * 2;
//     uint16_t row3 = DISPLAY_TEXT_Y + DISPLAY_TEXT_STEP * 3;
//     uint16_t row4 = DISPLAY_TEXT_Y + DISPLAY_TEXT_STEP * 4;
//     zf_device_ips200 *ips200 = display_image_get_device();
//
//     if(NULL == ips200 || NULL == chassis)
//     {
//         return;
//     }
//
//     if(NULL != vision)
//     {
//         // ERR : 车当前最关心的巡线误差，底部中线相对画面中心的偏差。
//         ips200->show_string(DISPLAY_TEXT_X, row0, "ERR");
//         ips200->show_int(DISPLAY_TEXT_X + 32, row0, (int32_t)vision->line_error, 4);
//
//         // REF : 最远参考列，用来作为本帧左右扫边的起始列。
//         ips200->show_string(DISPLAY_TEXT_COL2_X, row0, "REF");
//         ips200->show_uint(DISPLAY_TEXT_COL2_X + 32, row0, (uint32_t)vision->reference_col, 3);
//
//         // LOST : 是否整帧丢线，1 表示这帧没有可靠赛道。
//         ips200->show_string(DISPLAY_TEXT_X, row1, "LOST");
//         ips200->show_uint(DISPLAY_TEXT_X + 40, row1, (uint32_t)vision->lost_all, 1);
//
//         // HEAD : 远处中线和近处中线的差，反映赛道朝向变化。
//         ips200->show_string(DISPLAY_TEXT_COL2_X, row1, "HEAD");
//         ips200->show_float(DISPLAY_TEXT_COL2_X + 40, row1, vision->heading_error, 3, 1);
//     }
//
//     // TURN : 视觉外环算出来的左右轮差速量。
//     ips200->show_string(DISPLAY_TEXT_X, row2, "TURN");
//     ips200->show_int(DISPLAY_TEXT_X + 40, row2, (int32_t)chassis->turn_diff, 4);
//
//     // BASE : 直行时的基准速度目标，单位就是当前这套编码器每个控制周期的计数。
//     ips200->show_string(DISPLAY_TEXT_COL2_X, row2, "BASE");
//     ips200->show_int(DISPLAY_TEXT_COL2_X + 40, row2, (int32_t)chassis->base_speed, 4);
//
//     // LTAR / RTAR : 左右轮当前目标速度。
//     ips200->show_string(DISPLAY_TEXT_X, row3, "LTAR");
//     ips200->show_int(DISPLAY_TEXT_X + 40, row3, (int32_t)chassis->left_speed_target, 4);
//     ips200->show_string(DISPLAY_TEXT_COL2_X, row3, "RTAR");
//     ips200->show_int(DISPLAY_TEXT_COL2_X + 40, row3, (int32_t)chassis->right_speed_target, 4);
//
//     // LSPD / RSPD : 编码器修正方向后的测速值，直接拿来当轮速反馈。
//     ips200->show_string(DISPLAY_TEXT_X, row4, "LSPD");
//     ips200->show_int(DISPLAY_TEXT_X + 40, row4, (int32_t)chassis->left_speed, 4);
//     ips200->show_string(DISPLAY_TEXT_COL2_X, row4, "RSPD");
//     ips200->show_int(DISPLAY_TEXT_COL2_X + 40, row4, (int32_t)chassis->right_speed, 4);
//
//     // LPWM / RPWM : 当前左右电机实际输出的 PWM。
//     ips200->show_string(DISPLAY_TEXT_X, row4 + DISPLAY_TEXT_STEP, "LPWM");
//     ips200->show_uint(DISPLAY_TEXT_X + 40, row4 + DISPLAY_TEXT_STEP, (uint32_t)chassis->left_pwm, 4);
//     ips200->show_string(DISPLAY_TEXT_COL2_X, row4 + DISPLAY_TEXT_STEP, "RPWM");
//     ips200->show_uint(DISPLAY_TEXT_COL2_X + 40, row4 + DISPLAY_TEXT_STEP, (uint32_t)chassis->right_pwm, 4);
// }
