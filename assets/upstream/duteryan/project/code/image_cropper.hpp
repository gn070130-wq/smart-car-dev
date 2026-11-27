#ifndef IMAGE_CROPPER_HPP
#define IMAGE_CROPPER_HPP
#include "zf_common_headfile.hpp"

uint8_t* crop_gray_image(const uint8_t* src_gray);//裁剪灰度图像
uint8_t* compress_gray_image(const uint8_t* src_gray);//压缩灰度图像
uint16_t* crop_rgb565_image(const uint16_t* src_rgb);//裁剪RGB565图像
uint16_t* compress_rgb565_image(const uint16_t* src_rgb);//压缩RGB565图像
uint8_t* get_gray_ptr_from_color(const cv::Mat& color_img);//从彩色图像获取灰度图像指针
#endif // IMAGE_CROPPER_HPP