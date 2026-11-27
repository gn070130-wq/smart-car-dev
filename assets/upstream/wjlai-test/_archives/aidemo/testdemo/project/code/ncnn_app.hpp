#ifndef _NCNN_APP_HPP_
#define _NCNN_APP_HPP_

#include "ncnn_classifier.hpp"
#include "zf_device_uvc.hpp"
#include <opencv2/opencv.hpp>

// ==================== 分类模型配置 ====================
#define CLASSIFY_MODEL_DIR      "/home/root/class/model"
#define CLASSIFY_PARAM_FILE     CLASSIFY_MODEL_DIR "/tiny_classifier_fp16.ncnn.param"
#define CLASSIFY_BIN_FILE       CLASSIFY_MODEL_DIR "/tiny_classifier_fp16.ncnn.bin"
#define CLASSIFY_NUM_THREADS    1
#define CLASSIFY_INPUT_SIZE     48      // 模型输入尺寸（正方形）

static const int CLASSIFY_NUM_CLASSES = 3;
static const std::vector<std::string> CLASSIFY_CLASS_NAMES = {"supplies", "vehicles", "weapons"};

// 全局对象声明
extern demo::NcnnClassifier classifier;
extern zf_device_uvc        uvc_dev;

/**
 * @brief NCNN分类器初始化
 * @return 0=成功, -1=失败
 */
int ClassifierInit();

/**
 * @brief 摄像头初始化
 * @return 0=成功, -1=失败
 */
int CameraInit();

/**
 * @brief 使用输入图像进行推理，打印结果
 * @param input 输入BGR图像
 * @return true=推理成功, false=失败
 */
bool CaptureAndClassify(cv::Mat input);

#endif /* _NCNN_APP_HPP_ */
