#ifndef __SEEKFREE_ASSISTANT_HPP__
#define __SEEKFREE_ASSISTANT_HPP__

#include "zf_common_headfile.hpp"
#include "vision.hpp"

// 比赛时设为 0 关闭上位机，调试时改为 1。
#define ENABLE_SEEKFREE_ASSISTANT 0

int8 seeekfree_assistant_bind_tcp(zf_driver_tcp_client *client);
uint32 seekfree_assistant_transfer(const uint8 *buff, uint32 length);
uint32 seekfree_assistant_receive(uint8 *buff, uint32 length);

bool seekfree_assistant_debug_init(zf_driver_tcp_client *tcp_client, void *image_buffer, uint16 width, uint16 height);
void seekfree_assistant_debug_update_parameters(void);
bool seekfree_assistant_debug_should_send_camera(void);
void seekfree_assistant_debug_send_camera_frame(const uint8 *gray_image, uint8 image_copy[UVC_HEIGHT][UVC_WIDTH], const VisionResult &vision);
bool seekfree_assistant_debug_is_enabled(void);

#endif

