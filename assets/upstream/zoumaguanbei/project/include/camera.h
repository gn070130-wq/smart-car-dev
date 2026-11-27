#ifndef CAMERA_H
#define CAMERA_H

#include <iostream>
#include <cstdint>
#include <iomanip>
#include <opencv2/opencv.hpp>

// V4L2 核心头文件
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

using namespace std;
using namespace cv;

// ==================== 宏定义与全局常量 ====================
#define RESULT_ROW 240 // 结果图行数
#define RESULT_COL 320 // 结果图列数
#define USED_ROW   240 // 用于透视图的原始行数
#define USED_COL   320 // 用于透视图的原始列数

// ==================== 全局变量声明 ====================
extern int rows;
extern int cols;
extern uchar *p;               // 指向当前处理图像数据的指针
extern Mat frame;              // 原始彩色帧
extern Mat grayImage;          // 灰度图
extern Mat binaryImage;        // 二值图
extern Mat adjusted;
extern Mat g_yuyv;             // YUYV 原始图像 (CV_8UC2)，用于彩色逆透视
extern uint8_t *PerImg_ip_1[RESULT_ROW][RESULT_COL];
extern uint8_t *PerImg_ip[RESULT_ROW][RESULT_COL];
extern double camera_ipm_mat[3][3];
extern double camera_ipm_pitch;
extern double camera_ipm_scale;
extern int camera_ipm_offset_x;
extern int camera_ipm_offset_y;

// ==================== 基础摄像头函数 (VideoCapture) ====================
static VideoCapture cap;

void camera_init();            // 传统初始化：默认 320x240, MJPEG, 156fps
void camera_exposure_setting();

struct CameraExposureConfig {
    bool available;
    bool auto_exposure;
    int exposure_time;
    int exposure_min;
    int exposure_max;
    int exposure_step;
    bool auto_priority; // false: 帧率优先，true: 亮度优先
    bool auto_priority_available;
};

// 查询、应用并持久化 MMAP 摄像头曝光参数。
bool camera_exposure_get(CameraExposureConfig &config);
bool camera_exposure_set(bool auto_mode, int exposure_time, bool auto_priority,
                         CameraExposureConfig &applied, std::string &message);
bool camera_exposure_save(const CameraExposureConfig &config, std::string &message);

struct CameraControlValue {
    bool available;
    bool inactive;
    int value;
    int minimum;
    int maximum;
    int step;
};

struct CameraImageControls {
    CameraControlValue brightness;
    CameraControlValue contrast;
    CameraControlValue saturation;
    CameraControlValue hue;
    CameraControlValue auto_white_balance;
    CameraControlValue gamma;
    CameraControlValue power_line_frequency;
    CameraControlValue white_balance_temperature;
    CameraControlValue sharpness;
};

// 查询、整组应用并持久化网页中指定的九项画面参数。
bool camera_image_controls_get(CameraImageControls &controls);
bool camera_image_controls_set(const CameraImageControls &requested,
                               CameraImageControls &applied, std::string &message);
void camera_image_controls_load_saved();
void get_camera_message();
void get_camera_mmap_message();
int get_image(int setting);    // 1:二值化, 2:灰度, 3:彩色

// ==================== 图像处理与透视变换 ====================
void ImagePerspective1_Init();
void ImagePerspective2_Init();

void ImagePerspective1_Color_Init();
void ImagePerspective2_Color_Init();
void ImagePerspective3_Color_Init();
void camera_cfg_load();
void camera_cfg_save();
void camera_ipm_set_pitch(double pitch);
float getOtsuThreshold(const cv::Mat& grayImage);

// 像素访问接口
int get_image_sum(int y, int x);
int get_image_sum_real(int y, int x);
void set_image_sum(int y, int x, int num);
void set_image_sum_real(int y, int x, int num);

/**
 * @brief MMAP 模式初始化
 * 内部硬编码：320x240, 156fps, 自动尝试 YUYV -> MJPEG 降级处理
 */
void camera_mmap_init();

/**
 * @brief 获取一帧图像并处理
 * @param setting 1:基于 Y 的二值化, 2:Y 灰度, 3:彩色,
 *                4:基于白色得分的大津二值化, 5:白色得分灰度
 *                白色得分 = Y - (|U-128| + |V-128|) / 2
 *                (同步更新全局变量 p, rows, cols)
 */
int get_mmap_image(int setting);

/**
 * @brief 彻底清理摄像头资源
 */
void camera_mmap_clean();

void cut_image(int y_center, int x_center, int length);

void camera_mmap_perf_test();

void test();


struct HSV {
    float h;  // 色相 0~360
    float s;  // 饱和度 0~1
    float v;  // 明度 0~1
};

HSV yuyv_to_hsv(int y, int x);
#endif // CAMERA_H