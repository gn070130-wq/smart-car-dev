#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include <opencv2/opencv.hpp>
#include <string>

// 默认模型存放根路径
#define CAPTURE_MODEL_DIR   "/home/root/models/"

/**
 * @brief 分类结果结构体
 */
struct ClassifyResult {
    int   sort;          // 最佳分类 ID，失败时返回 -1
    float conf;          // 最佳分类的置信度 [0, 1]
    float prob[6];       // 各类别概率分布（最多 6 类）
};

/**
 * @brief 加载模型（通过 JSON 配置初始化）
 *
 * 模型目录结构（以 tiny_roadsign_net_opt 为例）：
 *   CAPTURE_MODEL_DIR/tiny_roadsign_net_opt/
 *   ├── tiny_roadsign_net_opt.json     ← 模型配置
 *   ├── tiny_roadsign_net_opt.param    ← 模型结构
 *   └── tiny_roadsign_net_opt.bin      ← 模型权重
 *
 * @param model_name  模型前缀名，如 "tiny_roadsign_net_opt"
 * @return            加载成功返回 true，失败返回 false
 */
bool model_load(const char* model_name);

/**
 * @brief 释放模型资源
 * 建议在程序结束前调用，重置内部模型状态
 */
void model_unload();

/** @brief 返回当前已成功加载的模型名称；未加载时为空字符串。 */
std::string current_model_name();

/**
 * @brief 模型推理
 *
 * 内部会自动处理：
 * 1. 内存连续性检查
 * 2. 图像缩放 (Resize) 至模型输入尺寸
 * 3. 颜色空间转换 (BGR -> RGB)
 * 4. 归一化及推理过程
 *
 * @param frame  输入帧（CV_8UC3 BGR 格式）
 * @return       ClassifyResult 结构体
 */
ClassifyResult model_identify(const cv::Mat& frame);

/** @brief 深拷贝并暂存一张 75x75、CV_8UC3 BGR 照片。 */
void add_photo(cv::Mat& frame);

/** @brief 将全部暂存照片按顺序保存到 /home/root/image/run/。 */
void save_all_photo();

/** @brief 清除全部暂存照片。函数名按现有接口要求保留。 */
void clean_ptoto();

/**
 * @brief 类别 ID 转为易读名称（从 JSON 配置读取）
 */
const char* class_name(int class_id);

/**
 * @brief 类别 ID 转为建议动作名称（从 JSON 配置读取）
 */
const char* action_name(int class_id);

#endif // CLASSIFIER_H
