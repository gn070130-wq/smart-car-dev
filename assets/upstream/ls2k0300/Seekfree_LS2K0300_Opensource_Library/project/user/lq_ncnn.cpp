#include "lq_ncnn.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <stdexcept>

LQ_NCNN::LQ_NCNN()
    : m_initialized(false)
    , m_input_width(64)
    , m_input_height(64)
    , m_input_name("in0")
    , m_output_name("out0")
{
    // ImageNet normalization used by the TinyClassifier training tutorial.
    m_mean_vals[0] = 123.675f;
    m_mean_vals[1] = 116.28f;
    m_mean_vals[2] = 103.53f;
    m_norm_vals[0] = 0.01712475f;
    m_norm_vals[1] = 0.017507f;
    m_norm_vals[2] = 0.01742919f;
}

LQ_NCNN::~LQ_NCNN() = default;

bool LQ_NCNN::Init()
{
    m_initialized = false;
    m_net.clear();
    m_net.opt.use_vulkan_compute = false;
    m_net.opt.num_threads = 1;

    if(m_param_path.empty() || 0 != m_net.load_param(m_param_path.c_str()))
    {
        std::fprintf(stderr, "NCNN: failed to load param file: %s\n", m_param_path.c_str());
        return false;
    }

    if(m_bin_path.empty() || 0 != m_net.load_model(m_bin_path.c_str()))
    {
        std::fprintf(stderr, "NCNN: failed to load model file: %s\n", m_bin_path.c_str());
        m_net.clear();
        return false;
    }

    m_initialized = true;
    std::printf("NCNN: model loaded: %s\n", m_param_path.c_str());
    return true;
}

std::string LQ_NCNN::Infer(const cv::Mat &bgr_image)
{
    return InferDetailed(bgr_image).label;
}

LQ_NCNN_Result LQ_NCNN::InferDetailed(const cv::Mat &bgr_image)
{
    if(!m_initialized)
    {
        throw std::runtime_error("NCNN is not initialized; call Init() first");
    }
    if(bgr_image.empty())
    {
        throw std::invalid_argument("NCNN input image is empty");
    }
    if(CV_8UC3 != bgr_image.type())
    {
        throw std::invalid_argument("NCNN input image must be CV_8UC3 BGR");
    }

    const cv::Mat source = bgr_image.isContinuous() ? bgr_image : bgr_image.clone();
    ncnn::Mat input = ncnn::Mat::from_pixels_resize(
        source.data,
        ncnn::Mat::PIXEL_BGR2RGB,
        source.cols,
        source.rows,
        m_input_width,
        m_input_height);
    if(input.empty())
    {
        throw std::runtime_error("NCNN failed to create input tensor");
    }
    input.substract_mean_normalize(m_mean_vals, m_norm_vals);

    const auto inference_start = std::chrono::steady_clock::now();
    ncnn::Extractor extractor = m_net.create_extractor();
    if(0 != extractor.input(m_input_name.c_str(), input))
    {
        throw std::runtime_error("NCNN failed to set input blob: " + m_input_name);
    }

    ncnn::Mat logits;
    if(0 != extractor.extract(m_output_name.c_str(), logits))
    {
        throw std::runtime_error("NCNN failed to extract output blob: " + m_output_name);
    }
    const auto inference_end = std::chrono::steady_clock::now();

    const int output_count = logits.w;
    if(1 != logits.dims || 1 != logits.elempack || sizeof(float) != logits.elemsize ||
       output_count != static_cast<int>(m_labels.size()))
    {
        throw std::runtime_error(
            "NCNN output must be a 1D FP32 tensor matching labels.txt; got " +
            std::to_string(output_count) + " values for " +
            std::to_string(m_labels.size()) + " labels");
    }

    float max_logit = -std::numeric_limits<float>::infinity();
    for(int index = 0; index < output_count; ++index)
    {
        if(!std::isfinite(logits[index]))
        {
            throw std::runtime_error("NCNN model returned a non-finite logit");
        }
        if(logits[index] > max_logit)
        {
            max_logit = logits[index];
        }
    }

    double probability_sum = 0.0;
    for(int index = 0; index < output_count; ++index)
    {
        probability_sum += std::exp(static_cast<double>(logits[index] - max_logit));
    }
    if(!std::isfinite(probability_sum) || probability_sum <= 0.0)
    {
        throw std::runtime_error("NCNN Softmax normalization failed");
    }

    const int class_id = Argmax(logits);
    if(class_id < 0)
    {
        throw std::runtime_error("NCNN model returned an empty output");
    }

    LQ_NCNN_Result result;
    result.class_id = class_id;
    result.label = m_labels[class_id];
    result.probabilities.resize((size_t)output_count);
    for(int index = 0; index < output_count; ++index)
    {
        result.probabilities[(size_t)index] = static_cast<float>(
            std::exp(static_cast<double>(logits[index] - max_logit)) / probability_sum);
    }
    result.confidence = result.probabilities[(size_t)class_id];
    result.inference_ms = std::chrono::duration<double, std::milli>(
        inference_end - inference_start).count();
    return result;
}

void LQ_NCNN::SetModelPath(const std::string &param_path, const std::string &bin_path)
{
    m_param_path = param_path;
    m_bin_path = bin_path;
}

void LQ_NCNN::SetInputSize(int width, int height)
{
    if(width <= 0 || height <= 0)
    {
        throw std::invalid_argument("NCNN input size must be positive");
    }
    m_input_width = width;
    m_input_height = height;
}

void LQ_NCNN::SetLabels(const std::vector<std::string> &labels)
{
    m_labels = labels;
}

bool LQ_NCNN::LoadLabels(const std::string &labels_path)
{
    std::ifstream labels_file(labels_path);
    if(!labels_file.is_open())
    {
        std::fprintf(stderr, "NCNN: failed to load labels file: %s\n", labels_path.c_str());
        return false;
    }

    std::vector<std::string> labels;
    std::string label;
    while(std::getline(labels_file, label))
    {
        if(!label.empty() && '\r' == label.back())
        {
            label.pop_back();
        }
        if(!label.empty())
        {
            labels.push_back(label);
        }
    }

    if(labels.empty())
    {
        std::fprintf(stderr, "NCNN: labels file is empty: %s\n", labels_path.c_str());
        return false;
    }

    m_labels = labels;
    return true;
}

const std::vector<std::string> &LQ_NCNN::GetLabels() const
{
    return m_labels;
}

void LQ_NCNN::SetNormalize(const float mean_vals[3], const float norm_vals[3])
{
    if(nullptr == mean_vals || nullptr == norm_vals)
    {
        throw std::invalid_argument("NCNN normalization arrays must not be null");
    }

    for(int channel = 0; channel < 3; ++channel)
    {
        m_mean_vals[channel] = mean_vals[channel];
        m_norm_vals[channel] = norm_vals[channel];
    }
}

int LQ_NCNN::Argmax(const ncnn::Mat &logits) const
{
    const int output_count = logits.w;
    if(output_count <= 0)
    {
        return -1;
    }

    int best_index = 0;
    float best_value = -std::numeric_limits<float>::infinity();
    for(int index = 0; index < output_count; ++index)
    {
        const float value = logits[index];
        if(value > best_value)
        {
            best_value = value;
            best_index = index;
        }
    }
    return best_index;
}
