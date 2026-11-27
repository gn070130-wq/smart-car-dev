#ifndef _DEAL_MODEL_H_
#define _DEAL_MODEL_H_

#include "zf_common_headfile.hpp"

/****************************** model ******************************/
#define  USE_GREEN_DETECT           (1)         // 是否使用绿色检测进行发车
#define  MIN_START_AERA             (2000.0)    // 最小发车面积阈值 2000.0 ( < 4000 )
#define  MIN_AREA                   (90.0)      // 最小色块面积 55cm 90.0
#define  MAX_AREA                   (840.0)     // 最大色块面积 15cm 840.0
#define  DEAL_MODEL_AREA            (170.0)     // 模型识别最小面积阈值 40cm 170.0
#define  MAX_MODEL_AREA             (600.0)     // 模型识别最大面积阈值 20cm 600.0
#define  CENTER_TOLERANCE           (0.1)       // 中心色块位置容差 10% 0.1
#define  ROI_MIN_AREA               (32 * 32)   // ROI最小面积（像素），过小可能是噪声
#define  DEVIATION_THRESHOLD_LOW    (10.0f)     // 偏差阈值(低等级)
#define  DEVIATION_THRESHOLD_HIGH   (25.0f)     // 偏差阈值(高等级)
#define  CONFIDENCE_THRESHOLD       (0.5f)      // 置信度阈值，低于此值的预测直接丢弃（0~1）
enum image_categories
{
    Supplies = 0,   // 物资
    Vehicle = 1,    // 载具
    Weapon = 2,     // 武器
};
enum image_subcategory
{
    Firearms = 0,                  // 武器
    Explosive = 1,                 // 爆炸物
    First_Aid_Kit = 2,             // 急救包
    Telescope = 3,                 // 望远镜
    Armoured = 4,                  // 装甲车
    Ambulance = 5,                 // 救护车
};
/****************************** filter ******************************/
#define  MAJORITY_FILTER_SIZE       (3) // 多数投票滤波窗口大小
/****************************** tensorflow ******************************/
#define  TFLITE_INPUT_WIDTH         (32) // tensorflow模型输入图像宽度（根据实际模型调整）
#define  TFLITE_INPUT_HEIGHT        (32) // tensorflow模型输入图像高度（根据实际模型调整）
#define  TFLITE_INPUT_CHANNEL       (3) // tensorflow模型输入图像通道数（RGB=3）
#define  TFLITE_OUTPUT_CLASS_NUM    (3) // tensorflow模型输出类别数（根据实际任务调整）
#define  TFLITE_INPUT_SIZE          (TFLITE_INPUT_WIDTH * TFLITE_INPUT_HEIGHT * TFLITE_INPUT_CHANNEL) // 输入总元素数
#define  TFLITE_OP_RESOLVER_MAX_NUM (20) // 算子解析器最大支持算子数
#define  TENSOR_ARENA_SIZE          (128 * 1024) // 张量空间大小（单位：字节，根据模型大小调整）
/****************************** NCNN ******************************/
#define  NCNN_INPUT_WIDTH           (32) // NCNN模型输入图像宽度（根据实际模型调整）
#define  NCNN_INPUT_HEIGHT          (32) // NCNN模型输入图像高度（根据实际模型调整）
#define  NCNN_OUTPUT_CLASS_NUM      (6) // NCNN模型输出类别数（根据实际任务调整）
#define  NCNN_NUM_THREADS           (1) // NCNN推理线程数
#define  NCNN_DEFAULT_PARAM_PATH    "tiny_classifier_fp32.ncnn.param" // 默认模型参数文件路径
#define  NCNN_DEFAULT_BIN_PATH      "tiny_classifier_fp32.ncnn.bin"   // 默认模型权重文件路径
/****************************** debug ******************************/
#define  LOG_PRINT_FREQUENCY        (1) // 每N帧打印一次推理结果（降低刷屏频率）
#define  ENABLE_FIRST_FRAME_DEBUG   (true) // 打印第一帧的完整预处理日志（调试用）
#define  MODEL_TESTING              (1)  // 模型测试标志位，0-禁用模型测试代码块 1-启用模型测试代码块 

// model
extern uint8_t model_type; // Weapon(left):1;Supplies(right):2;Vehicle:3
extern uint8_t last_model_type; // 上一次模型类型
extern const char* class_labels[]; // 模型类别标签数组（根据实际模型调整）
extern uint8_t model_x; // 模型中心x坐标
extern uint8_t model_y; // 模型中心y坐标
extern uint8_t model_coord_flag; // 模型坐标标志位
extern float red_block_area; // 红色色块面积
extern float model_area; // 模型面积
extern int16_t bottom_left_x, bottom_left_y, red_block_w, red_block_h;
struct MajorityVoteFilter // 多数投票滤波器结构体
{
    int16_t window_size;    // 滑动窗口大小
    int16_t max_classes;    // 类别总数（假设类别ID从0到max_classes-1）
    int16_t *history;       // 循环数组，存储窗口内的历史类别
    int16_t *counts;        // 统计各类别在当前窗口中的出现次数
    int16_t head;           // 循环队列的头部索引（下一个覆盖的位置）
    int16_t filled;         // 窗口已填充的数量（小于window_size时表示未满）
};

/****************************** tensorflow ******************************/
class TFLiteCameraClassifier // 模型处理类
{
public:
    TFLiteCameraClassifier(); // 构造函数
    ~TFLiteCameraClassifier(); // 析构函数
    void resetMajorityFilter(); //重置多数投票滤波器
    bool init(zf_device_uvc* uvc); // 初始化模型处理器，传入摄像头对象指针
    bool captureFrame(); // 捕获当前摄像头帧并进行预处理，准备输入模型
    bool runInference(); // 运行模型推理，返回是否成功
    int16_t getPredictionIndex() const { return max_index_; } // 获取预测类别索引
    float getPredictionConfidence() const { return max_confidence_; } // 获取预测类别置信度
    const char* getPredictionLabel() const { return class_labels[max_index_]; } // 获取预测类别标签
    int16_t getFrameCount() const { return frame_count_; } // 获取已处理帧数
    long long getLastInferenceTime() const { return last_inference_time_us_; } // 获取上次推理耗时（微秒）
    cv::Rect getRoiRect() const { return roi_rect_; }
    bool hasRoiRect() const { return roi_rect_.area() > 0; }
private:
    zf_device_uvc* uvc_ptr_; // 摄像头对象指针（用于获取图像数据）
    const tflite::Model* model_; // TFLite模型对象指针
    tflite::MicroMutableOpResolver<TFLITE_OP_RESOLVER_MAX_NUM> resolver_; // TFLite算子解析器
    uint8_t tensor_arena_[TENSOR_ARENA_SIZE]; // TFLite张量空间（静态分配）
    tflite::MicroInterpreter* interpreter_; // TFLite微型解释器对象指针
    cv::Mat src_img_; // 原始摄像头图像（BGR格式）
    cv::Mat resized_img_; // 模型输入尺寸的图像（BGR格式）
    cv::Mat float_img_; // 模型输入格式的图像（浮点数格式，归一化）
    cv::Mat roi_img_; // 模型输入的ROI图像（根据需要裁剪）
    cv::Rect roi_rect_; // ROI矩形定义（相对于原始图像坐标系）
    int16_t frame_count_; // 已处理帧数（用于日志控制）
    bool first_frame_logged_; // 是否已打印第一帧的预处理日志（调试用）
    long long last_inference_time_us_; // 上次推理耗时（微秒，用于性能监控）
    int16_t max_index_; // 模型输出的最大置信度类别索引
    float max_confidence_; // 模型输出的最大置信度值
    bool is_quantized_; // 模型是否使用int8量化
    float input_scale_; // 输入张量量化scale
    int32_t input_zero_point_; // 输入张量量化zero_point
    float output_scale_; // 输出张量量化scale
    int32_t output_zero_point_; // 输出张量量化zero_point
    bool preprocessImage(); // 图像预处理函数：从原始图像到模型输入格式的转换
    void parseOutput(); // 模型输出解析函数：从模型输出到预测结果的转换
    MajorityVoteFilter* majority_filter_; // 多数投票滤波器指针
};

/****************************** NCNN ******************************/
class NCNNCameraClassifier // NCNN模型处理类
{
public:
    NCNNCameraClassifier(); // 构造函数
    ~NCNNCameraClassifier(); // 析构函数
    void resetMajorityFilter(); //重置多数投票滤波器
    bool init(); // 初始化NCNN模型处理器
    bool captureFrame(); // 捕获当前摄像头帧，调用extract_model_roi提取ROI
    bool runInference(); // 运行模型推理，返回是否成功
    int16_t getPredictionIndex() const { return max_index_; } // 获取预测类别索引
    float getPredictionConfidence() const { return max_confidence_; } // 获取预测类别置信度
    const char* getPredictionLabel() const; // 获取预测类别标签
    int16_t getFrameCount() const { return frame_count_; } // 获取已处理帧数
    long long getLastInferenceTime() const { return last_inference_time_us_; } // 获取上次推理耗时（微秒）
    cv::Rect getRoiRect() const { return roi_rect_; }
    bool hasRoiRect() const { return roi_rect_.area() > 0; }
    void SetModelPath(const std::string& param_path, const std::string& bin_path); // 设置模型文件路径
    void SetInputSize(int width, int height);
    void SetLabels(const std::vector<std::string>& labels);
    void SetNormalize(const float mean_vals[3], const float norm_vals[3]);
private:
    bool preprocessImage(); // 图像预处理函数：缩放、BGR→RGB、from_pixels、归一化
    void parseOutput(const ncnn::Mat& logits); // 模型输出解析函数：Argmax + 多数投票滤波
    ncnn::Net                 net_;              // NCNN网络
    std::vector<std::string>  labels_;           // 类别标签
    bool                     initialized_;      // 初始化标志
    std::string              param_path_;
    std::string              bin_path_;
    int                      input_width_;
    int                      input_height_;
    float                    mean_vals_[3];
    float                    norm_vals_[3];
    std::string              input_name_;
    std::string              output_name_;
    MajorityVoteFilter*      majority_filter_;  // 多数投票滤波器指针
    cv::Mat src_img_; // 原始摄像头图像（BGR格式）
    cv::Mat resized_img_; // 模型输入尺寸的图像
    cv::Mat rgb_img_; // BGR转RGB后的图像
    cv::Mat roi_img_; // 模型输入的ROI图像（从src_img_裁剪）
    cv::Rect roi_rect_; // ROI矩形定义（相对于原始图像坐标系）
    int16_t frame_count_; // 已处理帧数
    bool first_frame_logged_; // 是否已打印第一帧的预处理日志（调试用）
    long long last_inference_time_us_; // 上次推理耗时（微秒）
    int16_t max_index_; // 模型输出的最大置信度类别索引
    float max_confidence_; // 模型输出的最大置信度值
    ncnn::Mat ncnn_input_; // NCNN输入矩阵（from_pixels + normalize后的结果）
};

/****************************** model ******************************/
bool detect_red_block(int16_t min_row, int16_t max_row, int16_t min_col, int16_t max_col); // 搜索红色色块
float detect_green_block(int16_t min_row, int16_t max_row, int16_t min_col, int16_t max_col); // 搜索绿色色块
void detect_model(void); // 搜索模型
bool extract_model_roi(const cv::Mat& src_bgr, cv::Mat& roi_img, cv::Rect& roi_rect); // 从全帧BGR图像中根据色块坐标计算并裁剪ROI
void majority_vote_free(MajorityVoteFilter *filter); // 释放多数投票滤波器内存

#endif
