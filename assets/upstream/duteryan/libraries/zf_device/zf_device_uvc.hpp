

#ifndef _ZF_DEVICE_UVC_HPP__
#define _ZF_DEVICE_UVC_HPP__

#include "zf_common_typedef.hpp"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstdint>
#include <turbojpeg.h>

// ==============================================
// 摄像头全局配置宏定义
// ==============================================
#define UVC_WIDTH         320
#define UVC_HEIGHT        240

// 建议先稳定 120FPS。
// 你的摄像头如果确认支持 180FPS，再改成 180。
#define UVC_FPS           10

#define UVC_PATH          "/dev/video0"

// 曝光模式：
// 1 = 手动曝光
// 3 = 自动曝光
extern int UVC_AUTO_EXPOSURE_MODE;
extern int UVC_DEFAULT_EXPOSURE;
extern int UVC_DEFAULT_GAIN;      // 增益:越低噪点越少(0=最低噪)
extern int UVC_POWER_LINE_FREQ;   // 防频闪:1=50Hz(国内) 2=60Hz 0=关

typedef enum
{
    UVC_AUTO_EXPOSURE_DISABLE = 1,
    UVC_AUTO_EXPOSURE_ENABLE  = 3,
} uvc_exposure_type_enum;

class zf_device_uvc
{
private:
    // ==========================================
    // 图像数据缓存
    // ==========================================
    cv::Mat frame_mjpg;     // 当前帧 BGR 彩色图像缓存
    cv::Mat frame_gray;     // 当前帧灰度图缓存
    cv::Mat frame_rgb;      // 当前帧 RGB565 缓存

    uint8_t* gray_image;
    uint16_t* rgb_image;

    // ==========================================
    // V4L2 底层变量
    // ==========================================
    int v4l2_fd;
    void* buffers[4];
    size_t buf_len[4];
    int buf_cnt;

    // ==========================================
    // TurboJPEG 解码器
    // ==========================================
    tjhandle tj_dec;

    // ==========================================
    // 当前帧 MJPEG 原始数据缓存
    // ==========================================
    std::vector<uint8_t> raw_jpeg_buffer;
    size_t raw_jpeg_size;

    // 当前帧懒加载状态
    bool is_gray_decoded;
    bool is_bgr_decoded;
    bool is_rgb_converted;

    // 实际生效的摄像头尺寸
    int actual_width;
    int actual_height;

private:
    int8 set_frame_rate(int fps);
    void discard_startup_frames(int count);
    void print_current_format_info();

public:
    uvc_exposure_type_enum uvc_exposure_type;

    zf_device_uvc();
    ~zf_device_uvc();

    // ==========================================
    // 核心接口
    // ==========================================
    int8 wait_image_refresh();

    // 获取当前帧灰度图裸指针。每帧最多解码一次灰度。
    uint8_t* get_gray_image_ptr();
    //获取当前彩色图像的指针
   uint16_t* get_rgb565_image_ptr();

    // 获取当前帧 BGR 图。每帧最多解码一次 BGR。
    cv::Mat get_frame_mjpg();

    // 获取当前帧 BGR ROI。
    // 注意：返回的是浅引用，不 clone。
    // 生命周期到下一次 wait_image_refresh() 之前有效。
    cv::Mat get_frame_mjpg_roi(int crop_x1, int crop_y1, int crop_x2, int crop_y2);

    // 如果后续需要 LCD 显示，可以再启用
    // uint16_t* get_rgb_image_ptr();

    // ==========================================
    // 摄像头参数控制
    // ==========================================
    bool is_camera_opened() const;

    int8 set_auto_exposure(int32_t auto_exposure_mode = UVC_AUTO_EXPOSURE_MODE);
    int8 set_exposure_value(int32_t exposure_value = UVC_DEFAULT_EXPOSURE);
    int8 set_gain(int32_t gain_value = UVC_DEFAULT_GAIN);                    // 增益(降噪)
    int8 set_power_line_frequency(int32_t freq = UVC_POWER_LINE_FREQ);       // 防频闪

    double get_auto_exposure_mode();
    double get_current_exposure();

    int8 init(const char *path = UVC_PATH);
};

#endif
