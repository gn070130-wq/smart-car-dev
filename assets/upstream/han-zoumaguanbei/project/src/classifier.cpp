#include "classifier.h"
#include "json.hpp"

#include <net.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <algorithm>
#include <mutex>

using json = nlohmann::json;

namespace {

// ---- JSON 配置中的模型参数 ----
struct ModelConfig {
    std::string name;
    int         input_width   = 64;
    int         input_height  = 64;
    int         num_classes   = 6;

    std::string param_file;
    std::string bin_file;
    std::string input_name    = "input";
    std::string output_name   = "prob";

    float       mean_vals[3]  = { 127.5f, 127.5f, 127.5f };
    float       norm_vals[3]  = { 1.0f / 127.5f, 1.0f / 127.5f, 1.0f / 127.5f };

    std::vector<std::string> class_names;
    std::vector<std::string> action_names;
};

// ---- 内部模型上下文 ----
struct ModelContext {
    ncnn::Net   net;
    ModelConfig config;
    bool        is_loaded   = false;

    ModelContext() {
        net.opt.use_vulkan_compute = false;
        net.opt.num_threads        = 1;
        net.opt.lightmode          = true;
    }
};

// 关键：使用唯一静态变量存储模型上下文
static ModelContext g_context;

static std::mutex g_model_mutex;
static std::string g_current_model_name;

constexpr int PHOTO_CACHE_CAPACITY = 100;
static cv::Mat g_photo_cache[PHOTO_CACHE_CAPACITY];
static int g_photo_count = 0;
static std::mutex g_photo_mutex;

bool file_exists(const std::string& path) {
    std::ifstream f(path.c_str(), std::ios::binary);
    return f.good();
}

/**
 * 从 JSON 文件中解析模型配置
 */
bool parse_model_config(const std::string& json_path, ModelConfig& cfg) {
    if (!file_exists(json_path)) {
        fprintf(stderr, "[model_load] JSON config not found: %s\n", json_path.c_str());
        return false;
    }

    try {
        std::ifstream ifs(json_path);
        json j;
        ifs >> j;

        cfg.name         = j.value("name", "");
        cfg.input_width  = j.value("input_width", 64);
        cfg.input_height = j.value("input_height", 64);
        cfg.num_classes  = j.value("num_classes", 6);

        cfg.param_file   = j.value("param_file", "");
        cfg.bin_file     = j.value("bin_file", "");
        cfg.input_name   = j.value("input_name", "input");
        cfg.output_name  = j.value("output_name", "prob");

        // 均值
        if (j.contains("mean_vals") && j["mean_vals"].is_array() && j["mean_vals"].size() >= 3) {
            cfg.mean_vals[0] = j["mean_vals"][0].get<float>();
            cfg.mean_vals[1] = j["mean_vals"][1].get<float>();
            cfg.mean_vals[2] = j["mean_vals"][2].get<float>();
        }
        // 归一化系数
        if (j.contains("norm_vals") && j["norm_vals"].is_array() && j["norm_vals"].size() >= 3) {
            cfg.norm_vals[0] = j["norm_vals"][0].get<float>();
            cfg.norm_vals[1] = j["norm_vals"][1].get<float>();
            cfg.norm_vals[2] = j["norm_vals"][2].get<float>();
        }

        // 类别名称
        if (j.contains("class_names") && j["class_names"].is_array()) {
            cfg.class_names.clear();
            for (const auto& name : j["class_names"])
                cfg.class_names.push_back(name.get<std::string>());
        }
        // 动作名称
        if (j.contains("action_names") && j["action_names"].is_array()) {
            cfg.action_names.clear();
            for (const auto& act : j["action_names"])
                cfg.action_names.push_back(act.get<std::string>());
        }

        return true;
    } catch (const std::exception& e) {
        fprintf(stderr, "[model_load] JSON parse error: %s\n", e.what());
        return false;
    }
}

} // namespace

// ============================================================
//  Public API 实现
// ============================================================

void add_photo(cv::Mat& frame) {
    std::lock_guard<std::mutex> photo_lock(g_photo_mutex);

    if (frame.empty()) {
        fprintf(stderr, "[add_photo] empty frame ignored\n");
        return;
    }
    if (frame.rows != 75 || frame.cols != 75 || frame.type() != CV_8UC3) {
        fprintf(stderr, "[add_photo] expected 75x75 CV_8UC3 BGR frame\n");
        return;
    }
    if (g_photo_count >= PHOTO_CACHE_CAPACITY) {
        fprintf(stderr, "[add_photo] photo cache is full (%d images)\n",
                PHOTO_CACHE_CAPACITY);
        return;
    }

    // 必须深拷贝，否则原始帧更新后缓存中的照片也会随之改变。
    g_photo_cache[g_photo_count++] = frame.clone();
}

void save_all_photo() {
    std::lock_guard<std::mutex> photo_lock(g_photo_mutex);
    const char* output_dir = "/home/root/image/run";
    char path[256];

    for (int i = 0; i < g_photo_count; ++i) {
        snprintf(path, sizeof(path), "%s/image%d.png", output_dir, i);
        try {
            if (!cv::imwrite(path, g_photo_cache[i])) {
                fprintf(stderr, "[save_all_photo] failed to save: %s\n", path);
            }
        } catch (const cv::Exception& e) {
            fprintf(stderr, "[save_all_photo] failed to save %s: %s\n",
                    path, e.what());
        }
    }
}

void clean_ptoto() {
    std::lock_guard<std::mutex> photo_lock(g_photo_mutex);

    for (int i = 0; i < g_photo_count; ++i) {
        g_photo_cache[i].release();
    }
    g_photo_count = 0;
}

bool model_load(const char* model_name) {
    std::lock_guard<std::mutex> model_lock(g_model_mutex);
    if (!model_name || !*model_name) return false;

    // 构建路径: CAPTURE_MODEL_DIR/前缀名/
    std::string model_dir = CAPTURE_MODEL_DIR;
    if (model_dir.back() != '/') model_dir += '/';
    model_dir += model_name;
    model_dir += '/';

    // JSON 配置文件路径
    const std::string json_path = model_dir + model_name + ".json";

    // 解析 JSON 配置
    ModelConfig cfg;
    if (!parse_model_config(json_path, cfg))
        return false;

    // 模型文件路径
    const std::string param_path = model_dir + cfg.param_file;
    const std::string bin_path   = model_dir + cfg.bin_file;

    if (!file_exists(param_path) || !file_exists(bin_path)) {
        fprintf(stderr, "[model_load] .param/.bin not found in: %s\n", model_dir.c_str());
        return false;
    }

    // 清理旧网络（如果重复加载）
    g_context.net.clear();
    g_context.is_loaded = false;

    if (g_context.net.load_param(param_path.c_str()) != 0 ||
        g_context.net.load_model(bin_path.c_str()) != 0) {
        fprintf(stderr, "[model_load] ncnn load failed\n");
        return false;
    }

    // 保存配置并标记加载成功
    g_context.config    = std::move(cfg);
    g_context.is_loaded = true;
    g_current_model_name = model_name;

    printf("[model_load] '%s' loaded successfully (%d classes, %dx%d)\n",
           model_name, g_context.config.num_classes,
           g_context.config.input_width, g_context.config.input_height);
    return true;
}

void model_unload() {
    std::lock_guard<std::mutex> model_lock(g_model_mutex);
    g_context.net.clear();
    g_context.is_loaded = false;
    g_current_model_name.clear();
}

std::string current_model_name() {
    std::lock_guard<std::mutex> model_lock(g_model_mutex);
    return g_current_model_name;
}

ClassifyResult model_identify(const cv::Mat& frame) {
    std::lock_guard<std::mutex> model_lock(g_model_mutex);
    ClassifyResult res = { -1, 0.0f, { 0.0f, 0.0f, 0.0f } };

    if (!g_context.is_loaded || frame.empty())
        return res;

    const ModelConfig& cfg = g_context.config;

    // 1. 确保输入连续性
    const cv::Mat input = frame;

    //cv::Mat input = frame.isContinuous() ? frame : frame.clone();

    // 2. 预处理：缩放 + BGR2RGB
    ncnn::Mat in = ncnn::Mat::from_pixels_resize(
    input.data, ncnn::Mat::PIXEL_BGR,
    input.cols, input.rows, (int)input.step, 
    cfg.input_width, cfg.input_height, 0); // 补上最后的 0

    // 3. 归一化
    in.substract_mean_normalize(cfg.mean_vals, cfg.norm_vals);

    // 4. 推理
    ncnn::Extractor ex = g_context.net.create_extractor();
    if (ex.input(cfg.input_name.c_str(), in) != 0)
        return res;

    ncnn::Mat out;
    if (ex.extract(cfg.output_name.c_str(), out) != 0)
        return res;

    if (out.w * out.h * out.c != cfg.num_classes)
        return res;

    // 5. 后处理：取最大概率类别
    const float* ptr = out;
    int   best_idx  = 0;
    float best_prob = -1.0f;

    for (int i = 0; i < cfg.num_classes; ++i) {
        if (i < 6) res.prob[i] = ptr[i];  // 安全写入 prob 数组
        if (ptr[i] > best_prob) {
            best_prob = ptr[i];
            best_idx  = i;
        }
    }

    res.sort = best_idx;
    res.conf = best_prob;
    return res;
}

const char* class_name(int class_id) {
    const auto& names = g_context.config.class_names;
    if (class_id < 0 || class_id >= static_cast<int>(names.size()))
        return "unknown";
    return names[class_id].c_str();
}

const char* action_name(int class_id) {
    const auto& acts = g_context.config.action_names;
    if (class_id < 0 || class_id >= static_cast<int>(acts.size()))
        return "unknown";
    return acts[class_id].c_str();
}
