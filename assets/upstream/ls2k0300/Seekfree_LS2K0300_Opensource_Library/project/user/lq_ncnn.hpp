#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <ncnn/net.h>

struct LQ_NCNN_Result
{
    int class_id;
    std::string label;
    std::vector<float> probabilities;
    float confidence;
    double inference_ms;
};

// NCNN image-classification wrapper ported from the LoongQiu library.
class LQ_NCNN
{
public:
    LQ_NCNN();
    ~LQ_NCNN();

    // Configure the model before calling Init().
    void SetModelPath(const std::string &param_path, const std::string &bin_path);
    void SetInputSize(int width, int height);
    void SetLabels(const std::vector<std::string> &labels);
    bool LoadLabels(const std::string &labels_path);
    const std::vector<std::string> &GetLabels() const;
    void SetNormalize(const float mean_vals[3], const float norm_vals[3]);

    // Loads the .param and .bin files. Returns false when loading fails.
    bool Init();

    // Input must be an OpenCV BGR image. Returns the label, or the class id
    // as text when no matching label was configured.
    std::string Infer(const cv::Mat &bgr_image);
    LQ_NCNN_Result InferDetailed(const cv::Mat &bgr_image);

private:
    int Argmax(const ncnn::Mat &logits) const;

    ncnn::Net m_net;
    std::vector<std::string> m_labels;
    bool m_initialized;

    std::string m_param_path;
    std::string m_bin_path;
    int m_input_width;
    int m_input_height;
    float m_mean_vals[3];
    float m_norm_vals[3];

    // pnnx uses these names for the TinyClassifier model from the tutorial.
    std::string m_input_name;
    std::string m_output_name;
};
