/*
 * NCNN classifier implementation (tiny_classifier_fp16_64.* default).
 */

#include "ncnn_classifier.hpp"
#include "net.h"

namespace demo
{
struct NcnnClassifier::Impl
{
    ncnn::Net net;
};

NcnnClassifier::NcnnClassifier()
{
    m_impl = new Impl();
}

NcnnClassifier::~NcnnClassifier()
{
    delete m_impl;
    m_impl = nullptr;
}

int NcnnClassifier::init(const std::string& param_path,
                         const std::string& bin_path,
                         int input_width,
                         int input_height,
                         int num_threads,
                         bool use_vulkan)
{
    if (!m_impl) return -1;
    if (param_path.empty() || bin_path.empty()) return -1;

    m_impl->net.opt.use_vulkan_compute = use_vulkan;
    m_impl->net.opt.num_threads = (num_threads > 0) ? num_threads : 1;

    if (m_impl->net.load_param(param_path.c_str()) != 0) return -1;
    if (m_impl->net.load_model(bin_path.c_str()) != 0) return -1;

    // 设置输入尺寸
    m_input_w = (input_width > 0) ? input_width : 64;
    m_input_h = (input_height > 0) ? input_height : 64;

    m_inited = true;
    return 0;
}

int NcnnClassifier::argmax(const std::vector<float>& v)
{
    if (v.empty()) return -1;
    int best_i = 0;
    float best_v = v[0];
    for (int i = 1; i < (int)v.size(); ++i)
    {
        if (v[i] > best_v)
        {
            best_v = v[i];
            best_i = i;
        }
    }
    return best_i;
}

void NcnnClassifier::softmax_inplace(std::vector<float>& v)
{
    if (v.empty()) return;

    float maxv = -std::numeric_limits<float>::infinity();
    for (float x : v) maxv = (x > maxv) ? x : maxv;

    float sum = 0.f;
    for (float& x : v)
    {
        x = std::exp(x - maxv);
        sum += x;
    }

    if (sum <= 0.f) return;
    const float inv = 1.f / sum;
    for (float& x : v) x *= inv;
}

bool NcnnClassifier::classify_bgr(const cv::Mat& bgr, ClassifyResult& out) const
{
    if (!m_inited || !m_impl) return false;
    if (bgr.empty()) return false;
    if (bgr.type() != CV_8UC3) return false;

    cv::Mat resized;
    if (bgr.cols != m_input_w || bgr.rows != m_input_h)
        cv::resize(bgr, resized, cv::Size(m_input_w, m_input_h));
    else
        resized = bgr;

    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    ncnn::Mat in = ncnn::Mat::from_pixels(
        rgb.data, ncnn::Mat::PIXEL_RGB, m_input_w, m_input_h);
    in.substract_mean_normalize(m_mean_vals, m_norm_vals);

    ncnn::Extractor ex = m_impl->net.create_extractor();
    ex.set_light_mode(true);
    ex.input(m_input_blob.c_str(), in);

    ncnn::Mat logits;
    if (ex.extract(m_output_blob.c_str(), logits) != 0) return false;

    out.probs.assign(logits.w, 0.f);
    for (int i = 0; i < logits.w; ++i) out.probs[i] = logits[i];
    softmax_inplace(out.probs);

    out.class_id = argmax(out.probs);
    out.confidence = (out.class_id >= 0 && out.class_id < (int)out.probs.size()) ? out.probs[out.class_id] : 0.f;
    return (out.class_id >= 0);
}
} // namespace demo
