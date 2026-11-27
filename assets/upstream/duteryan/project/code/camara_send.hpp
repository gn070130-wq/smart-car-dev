#ifndef __CARAMA_SEND_HPP__
#define __CARAMA_SEND_HPP__
#include  "zf_common_headfile.hpp"
#define STR_BUF_LEN    6      // 每个数字的字符串缓冲区（65535+结束符）
#define SEND_WIDTH (320)
#define SEND_HEIGHT (160)
#define DETECT_WIDTH (40)
#define DETECT_HEIGHT (40)
extern int jpg_cnt;
extern zf_driver_tcp_client tcp_client_dev;
extern  uint16_t* rgb_image;
// 另外一端的IP地址
#define SERVER_IP "192.168.3.4"
// 端口号
#define PORT 8088
extern uint16 image_copy[SEND_HEIGHT][SEND_WIDTH];
extern uint16 image_detect[DETECT_HEIGHT][DETECT_WIDTH];
void carama_send_init();
void rgb_image_send(const uint16_t* src_rgb);
void crop_image(const uint16_t* src,int x0, int y0, int x1, int y1,uint16 dst[DETECT_HEIGHT][DETECT_WIDTH]);//裁剪rgb图像的对应位置
uint32 tcp_send_wrap(const uint8 *buf, uint32 len);
uint32 tcp_send_wrap(const uint8 *buf, uint32 len);
void Save_RGB565_To_JPG_ID(uint16 image_detect[DETECT_HEIGHT][DETECT_WIDTH],const char* base_name,int id);
void capture_save_roi_mjpg(const char* name_prefix, int custom_id);
void capture_save_raw_roi(const char* name_prefix, int custom_id);   // 存原始80x80彩色ROI(未处理)
void oscilloscope_send_data();//示波器传数据
#endif

