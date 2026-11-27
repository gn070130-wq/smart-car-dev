#include "image_recognition.hpp"

#include <cstdio>
#include <exception>
#include <stdexcept>

#include "lq_ncnn.hpp"

namespace
{
constexpr int MODEL_INPUT_WIDTH = 64;
constexpr int MODEL_INPUT_HEIGHT = 64;
constexpr int MODEL_CLASS_COUNT = 3;
constexpr char MODEL_PARAM_PATH[] = "/home/root/tiny_classifier/model/tiny_classifier_fp32.ncnn.param";
constexpr char MODEL_BIN_PATH[] = "/home/root/tiny_classifier/model/tiny_classifier_fp32.ncnn.bin";
constexpr char MODEL_LABELS_PATH[] = "/home/root/tiny_classifier/model/labels.txt";

LQ_NCNN classifier;
bool classifier_initialized = false;

struct LabelDescription
{
    const char *label;
    const char *type_name;
    const char *action_name;
    RedAvoidanceDirection direction;
};

constexpr LabelDescription LABEL_DESCRIPTIONS[] = {
    {"supplies", "补给", "右转", RED_AVOIDANCE_DIRECTION_RIGHT},
    {"vehicle",  "车辆", "直行", RED_AVOIDANCE_DIRECTION_STRAIGHT},
    {"weapon",   "武器", "左转", RED_AVOIDANCE_DIRECTION_LEFT},
};

bool build_model_input(const cv::Mat &original_frame,
                       const VisionResult &vision_result,
                       cv::Mat &model_input)
{
    if(original_frame.empty() || 0 == vision_result.image_found)
    {
        return false;
    }

    const float scale_x = static_cast<float>(original_frame.cols) /
                          static_cast<float>(SEARCH_UVC_WIDTH);
    const float scale_y = static_cast<float>(original_frame.rows) /
                          static_cast<float>(SEARCH_UVC_HEIGHT);

    if(0 == vision_result.image_w || 0 == vision_result.image_h)
    {
        return false;
    }

    const int roi_left = cvRound(vision_result.image_x * scale_x);
    const int roi_top = cvRound(vision_result.image_y * scale_y);
    const int roi_right = cvRound((vision_result.image_x + vision_result.image_w) * scale_x);
    const int roi_bottom = cvRound((vision_result.image_y + vision_result.image_h) * scale_y);
    const cv::Rect roi(roi_left,
                       roi_top,
                       roi_right - roi_left,
                       roi_bottom - roi_top);
    const cv::Rect frame_bounds(0, 0, original_frame.cols, original_frame.rows);
    if(roi.width <= 0 || roi.height <= 0 || (roi & frame_bounds) != roi)
    {
        return false;
    }

    model_input = original_frame(roi).clone();
    return model_input.cols == MODEL_INPUT_WIDTH &&
           model_input.rows == MODEL_INPUT_HEIGHT;
}

} // namespace

bool image_recognition_init(void)
{
    classifier.SetModelPath(MODEL_PARAM_PATH, MODEL_BIN_PATH);
    classifier.SetInputSize(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT);

    if(!classifier.LoadLabels(MODEL_LABELS_PATH))
    {
        return false;
    }

    const std::vector<std::string> &labels = classifier.GetLabels();
    if(labels.size() != (size_t)MODEL_CLASS_COUNT)
    {
        std::fprintf(stderr,
                     "NCNN: expected %d labels, got %zu\n",
                     MODEL_CLASS_COUNT,
                     labels.size());
        return false;
    }
    for(int index = 0; index < MODEL_CLASS_COUNT; ++index)
    {
        if(labels[(size_t)index] != LABEL_DESCRIPTIONS[index].label)
        {
            std::fprintf(stderr,
                         "NCNN: label %d must be %s, got %s\n",
                         index,
                         LABEL_DESCRIPTIONS[index].label,
                         labels[(size_t)index].c_str());
            return false;
        }
    }

    classifier_initialized = classifier.Init();
    return classifier_initialized;
}

bool image_recognition_infer(const cv::Mat &original_frame,
                             const VisionResult &vision_result,
                             ImageRecognitionResult &result)
{
    if(!classifier_initialized)
    {
        return false;
    }

    cv::Mat model_input;
    if(!build_model_input(original_frame, vision_result, model_input))
    {
        return false;
    }
    try
    {
        const LQ_NCNN_Result ncnn_result = classifier.InferDetailed(model_input);
        result.class_id = ncnn_result.class_id;
        result.label = ncnn_result.label;
        if(ncnn_result.probabilities.size() != result.probabilities.size())
        {
            throw std::runtime_error("NCNN expected exactly three Softmax probabilities");
        }
        for(size_t index = 0; index < result.probabilities.size(); ++index)
        {
            result.probabilities[index] = ncnn_result.probabilities[index];
        }
        result.confidence = ncnn_result.confidence;
        result.inference_ms = ncnn_result.inference_ms;
        result.direction = LABEL_DESCRIPTIONS[result.class_id].direction;
        return true;
    }
    catch(const std::exception &error)
    {
        std::fprintf(stderr, "NCNN inference failed: %s\n", error.what());
        return false;
    }
}

void image_recognition_print(const ImageRecognitionResult &result)
{
    const LabelDescription &description = LABEL_DESCRIPTIONS[result.class_id];
    std::printf("[NCNN] 类型=%s(%s), 动作=%s, 置信度=%.2f%%, "
                "概率={supplies=%.2f%%, vehicle=%.2f%%, weapon=%.2f%%}, "
                "推理耗时=%.3f ms\n",
                description.type_name,
                result.label.c_str(),
                description.action_name,
                result.confidence * 100.0f,
                result.probabilities[0] * 100.0f,
                result.probabilities[1] * 100.0f,
                result.probabilities[2] * 100.0f,
                result.inference_ms);
}
