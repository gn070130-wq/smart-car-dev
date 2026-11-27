#include "Seeekfree_assistant.hpp"
#include "chassis.hpp"

#include <cstring>

#define ASSISTANT_TCP_SERVER_IP     "172.26.215.250"
#define ASSISTANT_TCP_SERVER_PORT   8086
#define ASSISTANT_SEND_DIV          (UVC_FPS / 15)

static zf_driver_tcp_client *g_assistant_tcp_client = nullptr;
static bool g_assistant_debug_enabled = false;
static uint8 assistant_send_count = 0;

// 逐飞助手会保存这些数组地址，因此边界点数组必须使用静态存储，不能定义在栈上。
static uint8 left_x[UVC_HEIGHT];
static uint8 left_y[UVC_HEIGHT];
static uint8 center_x[UVC_HEIGHT];
static uint8 center_y[UVC_HEIGHT];
static uint8 right_x[UVC_HEIGHT];
static uint8 right_y[UVC_HEIGHT];

static uint8 assistant_camera_send_div(void)
{
    return (ASSISTANT_SEND_DIV > 0) ? ASSISTANT_SEND_DIV : 1;
}

static void draw_line_on_gray_copy(uint8 image_copy[UVC_HEIGHT][UVC_WIDTH], uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color)
{
    int x = x0;
    int y = y0;
    int end_x = x1;
    int end_y = y1;
    int dx = (end_x > x) ? (end_x - x) : (x - end_x);
    int dy = (end_y > y) ? (end_y - y) : (y - end_y);
    int step_x = (x < end_x) ? 1 : -1;
    int step_y = (y < end_y) ? 1 : -1;
    int err = dx - dy;

    while(true)
    {
        image_copy[y][x] = color;
        if(x == end_x && y == end_y)
        {
            break;
        }
        int err2 = err * 2;
        if(err2 > -dy)
        {
            err -= dy;
            x += step_x;
        }
        if(err2 < dx)
        {
            err += dx;
            y += step_y;
        }
    }
}

static void draw_quad_on_gray_copy(uint8 image_copy[UVC_HEIGHT][UVC_WIDTH], const uint8_t *corner_x, const uint8_t *corner_y, uint8_t color)
{
    if(nullptr == corner_x || nullptr == corner_y)
    {
        return;
    }

    for(uint8_t index = 0; index < 4; index++)
    {
        uint8_t next = (uint8_t)((index + 1) % 4);
        draw_line_on_gray_copy(image_copy, corner_x[index], corner_y[index], corner_x[next], corner_y[next], color);
    }
}

static void draw_detection_overlay_on_gray_copy(uint8 image_copy[UVC_HEIGHT][UVC_WIDTH], const VisionResult &vision)
{
    if(vision.image_found)
    {
        draw_quad_on_gray_copy(image_copy, vision.image_corner_x, vision.image_corner_y, 0);
    }
    if(vision.red_found)
    {
        draw_quad_on_gray_copy(image_copy, vision.red_corner_x, vision.red_corner_y, 255);
    }
}

static void update_assistant_boundary_overlay(const VisionResult &vision)
{
    if(vision.valid_row_start >= UVC_HEIGHT || vision.valid_row_end >= UVC_HEIGHT ||
       vision.valid_row_start > vision.valid_row_end)
    {
        seekfree_assistant_camera_boundary_config(NO_BOUNDARY, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        return;
    }

    uint16 point_count = (uint16)((vision.valid_row_end - vision.valid_row_start) / CONTRASTOFFSET + 1);

    // XY_BOUNDARY 只发送真实采样行，避免把未采样行的旧值也发给上位机。
    for(uint16 index = 0; index < point_count; index++)
    {
        uint8 row = (uint8)(vision.valid_row_start + index * CONTRASTOFFSET);
        left_x[index] = vision.left_boundary[row];
        left_y[index] = row;
        center_x[index] = vision.center_line[row];
        center_y[index] = row;
        right_x[index] = vision.right_boundary[row];
        right_y[index] = row;
    }

    seekfree_assistant_camera_boundary_config(XY_BOUNDARY, point_count,
                                              left_x, center_x, right_x,
                                              left_y, center_y, right_y);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     逐飞助手绑定TCP客户端
// 参数说明     *client         已完成 init 的 TCP 客户端对象
// 返回参数     int8            0-绑定成功  -1-参数无效
//-------------------------------------------------------------------------------------------------------------------
int8 seeekfree_assistant_bind_tcp(zf_driver_tcp_client *client)
{
    if(nullptr == client)
    {
        g_assistant_tcp_client = nullptr;
        return -1;
    }

    g_assistant_tcp_client = client;
    seekfree_assistant_interface_init(seekfree_assistant_transfer, seekfree_assistant_receive);

    return 0;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     逐飞助手发送函数
// 参数说明     *buff           需要发送的数据地址
// 参数说明     length          需要发送的长度
// 返回参数     uint32          剩余未发送数据长度
//-------------------------------------------------------------------------------------------------------------------
uint32 seekfree_assistant_transfer(const uint8 *buff, uint32 length)
{
    if(nullptr == g_assistant_tcp_client || nullptr == buff || 0 == length)
    {
        return 0;
    }

    return g_assistant_tcp_client->send_data(buff, length);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     逐飞助手接收数据函数
// 参数说明     *buff           需要接收的数据地址
// 参数说明     length          要接收的数据最大长度
// 返回参数     uint32          接收到的数据长度
//-------------------------------------------------------------------------------------------------------------------
uint32 seekfree_assistant_receive(uint8 *buff, uint32 length)
{
    if(nullptr == g_assistant_tcp_client || nullptr == buff || 0 == length)
    {
        return 0;
    }

    return g_assistant_tcp_client->read_data(buff, length);
}

bool seekfree_assistant_debug_init(zf_driver_tcp_client *tcp_client, void *image_buffer, uint16 width, uint16 height)
{
    g_assistant_debug_enabled = false;
    assistant_send_count = 0;
    g_assistant_tcp_client = nullptr;

    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, image_buffer, width, height);

    if(nullptr == tcp_client)
    {
        printf("seekfree assistant tcp client invalid, assistant disabled\r\n");
        return false;
    }

    if(0 != tcp_client->init(ASSISTANT_TCP_SERVER_IP, ASSISTANT_TCP_SERVER_PORT))
    {
        printf("tcp client init failed, seekfree assistant disabled\r\n");
        return false;
    }

    if(0 != seeekfree_assistant_bind_tcp(tcp_client))
    {
        printf("seekfree assistant bind failed, assistant disabled\r\n");
        return false;
    }

    vision_sync_reference_contrast_to_assistant();
    g_assistant_debug_enabled = true;
    printf("seekfree assistant connected: %s:%d\r\n", ASSISTANT_TCP_SERVER_IP, ASSISTANT_TCP_SERVER_PORT);

    return true;
}

void seekfree_assistant_debug_update_parameters(void)
{
    if(!g_assistant_debug_enabled)
    {
        return;
    }

    seekfree_assistant_data_analysis();
    chassis_update_vision_pid_from_assistant();
    vision_update_reference_contrast_from_assistant();
}

bool seekfree_assistant_debug_should_send_camera(void)
{
    if(!g_assistant_debug_enabled)
    {
        return false;
    }

    assistant_send_count++;
    if(assistant_send_count < assistant_camera_send_div())
    {
        return false;
    }

    assistant_send_count = 0;
    return true;
}

void seekfree_assistant_debug_send_camera_frame(const uint8 *gray_image, uint8 image_copy[UVC_HEIGHT][UVC_WIDTH], const VisionResult &vision)
{
    if(!g_assistant_debug_enabled || nullptr == gray_image || nullptr == image_copy)
    {
        return;
    }

    memcpy(image_copy[0], gray_image, UVC_WIDTH * UVC_HEIGHT);
    draw_detection_overlay_on_gray_copy(image_copy, vision);
    update_assistant_boundary_overlay(vision);
    seekfree_assistant_camera_send();
}

bool seekfree_assistant_debug_is_enabled(void)
{
    return g_assistant_debug_enabled;
}
