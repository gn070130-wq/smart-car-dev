/*
 * Simple NCNN image classifier wrapper for LS2K0300 demo.
 *
 * Keeps dependencies minimal: OpenCV for preprocessing + NCNN for inference.
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <limits>

namespace demo
{
struct ClassifyResult
{
    int class_id = -1;
    float confidence = 0.0f;
    std::vector<float> probs;
};

class NcnnClassifier
{
public:
    NcnnClassifier();
    ~NcnnClassifier();

    // Returns 0 on success, -1 on failure.
    int init(const std::string& param_path,
             const std::string& bin_path,
             int input_width = 64,
             int input_height = 64,
             int num_threads = 1,
             bool use_vulkan = false);

    bool is_inited() const { return m_inited; }

    // Expects BGR input (OpenCV default).
    // Returns true on success.
    bool classify_bgr(const cv::Mat& bgr, ClassifyResult& out) const;

    int input_width() const { return m_input_w; }
    int input_height() const { return m_input_h; }

private:
    static int argmax(const std::vector<float>& v);
    static void softmax_inplace(std::vector<float>& v);

private:
    // Opaque to header users; defined in .cpp to keep includes local.
    struct Impl;
    Impl* m_impl;

    bool m_inited = false;
    int m_input_w = 64;
    int m_input_h = 64;

    std::string m_input_blob = "in0";
    std::string m_output_blob = "out0";

    float m_mean_vals[3] = {123.675f, 116.28f, 103.53f};
    float m_norm_vals[3] = {0.01712475f, 0.017507f, 0.01742919f};
};
} // namespace demo
