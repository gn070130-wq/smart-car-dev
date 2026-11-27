#include "ncnn_app.hpp"
#include <cstdio>
#include <chrono>

// 全局对象定义
demo::NcnnClassifier classifier;
zf_device_uvc        uvc_dev;

/**
 * @brief NCNN分类器初始化
 */
int ClassifierInit()
{
    printf("Loading NCNN classifier model...\n");
    printf("  param: %s\n", CLASSIFY_PARAM_FILE);
    printf("  bin:   %s\n", CLASSIFY_BIN_FILE);

    if (classifier.init(CLASSIFY_PARAM_FILE, CLASSIFY_BIN_FILE,
                        CLASSIFY_INPUT_SIZE, CLASSIFY_INPUT_SIZE,
                        CLASSIFY_NUM_THREADS) != 0)
    {
        printf("NCNN classifier initialization failed!\n");
        return -1;
    }

    printf("NCNN classifier initialized: input=%dx%d, classes=%d\n",
           classifier.input_width(), classifier.input_height(), CLASSIFY_NUM_CLASSES);
    return 0;
}

/**
 * @brief 摄像头初始化
 */
int CameraInit()
{
    if (uvc_dev.init() != 0)
    {
        printf("UVC camera initialization failed!\n");
        return -1;
    }
    printf("UVC camera initialized: %dx%d\n", UVC_WIDTH, UVC_HEIGHT);
    return 0;
}

/**
 * @brief 从BGR图像中心裁切为正方形
 * @param src 输入BGR图像
 * @return 裁切后的正方形图像
 */
static cv::Mat center_crop_square(const cv::Mat& src)
{
    int w = src.cols;
    int h = src.rows;
    int crop_size = std::min(w, h);

    int x = (w - crop_size) / 2;
    int y = (h - crop_size) / 2;

    return src(cv::Rect(x, y, crop_size, crop_size)).clone();
}

/**
 * @brief 采集一帧并推理
 */
bool CaptureAndClassify(cv::Mat input)
{
    // 检查输入图像是否为空
    if (input.empty())
    {
        printf("[Classify] Empty input image\n");
        return false;
    }

    // 推理（classify_bgr内部会resize到模型输入尺寸）
    auto t0 = std::chrono::steady_clock::now();

    demo::ClassifyResult result;
    bool ok = classifier.classify_bgr(input, result);

    auto t1 = std::chrono::steady_clock::now();
    float infer_ms = std::chrono::duration<float, std::milli>(t1 - t0).count();

    if (!ok)
    {
        printf("[Classify] Inference failed\n");
        return false;
    }

    // 打印结果
    const char* class_name = (result.class_id >= 0 && result.class_id < CLASSIFY_NUM_CLASSES)
                             ? CLASSIFY_CLASS_NAMES[result.class_id].c_str()
                             : "unknown";

    printf("[Classify] class=%s(%d) conf=%.2f%% infer=%.1fms\n",
           class_name, result.class_id, result.confidence * 100.f, infer_ms);

    return true;
}
