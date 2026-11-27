#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <ncnn/net.h>
#include "scene_detect.h"  // for TargetCategory

/* ═══════════════════════════ 检测结果 ═══════════════════════════════ */

struct DetectObject {
    cv::Rect_<float> rect;
    int   label;
    float prob;
};

/* ═══════════════════════════ YOLO 检测器类 ═══════════════════════════ */
/*
 * 封装为类，避免每帧重新加载模型
 * 初始化一次，多次调用 detect()
 */

class YoloDetector {
public:
    YoloDetector();
    ~YoloDetector();

    /**
     * @brief 加载 ncnn 模型
     * @param param_path  .ncnn.param 文件路径
     * @param model_path  .ncnn.bin 文件路径
     * @return 0 成功, -1 失败
     */
    int load_model(const std::string& param_path,
                   const std::string& model_path);

    /**
     * @brief 检测单张图像
     * @param bgr        输入 BGR 图像
     * @param objects    输出检测结果列表
     * @param prob_thresh 置信度阈值
     * @param nms_thresh  NMS阈值
     * @return 0 成功
     */
    int detect(const cv::Mat& bgr,
               std::vector<DetectObject>& objects,
               float prob_thresh = 0.25f,
               float nms_thresh  = 0.45f);

    /**
     * @brief 将检测结果映射为目标板类别
     *
     * 根据第21届智能车竞赛数据集：
     *   武器类 (枪支、爆炸物、匕首、警棍、消防斧) → TARGET_WEAPON
     *   物资类 (急救包、手电筒、对讲机、防弹背心、望远镜、头盔) → TARGET_SUPPLY
     *   交通工具类 (消防车、救护车、装甲车、摩托车) → TARGET_VEHICLE
     */
    TargetCategory classify_target(const std::vector<DetectObject>& objects);

    bool is_loaded() const { return model_loaded; }

private:
    ncnn::Net net;
    bool model_loaded;
    int  target_size;

    /* 类别名到大类的映射（在 load 时建立） */
    std::vector<TargetCategory> label_to_category;
    void build_category_map();
};
