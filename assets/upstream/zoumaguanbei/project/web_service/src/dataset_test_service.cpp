#include "dataset_test_service.h"
#include "classifier.h"

#include <opencv2/imgcodecs.hpp>
#include "json.hpp"
#include <fstream>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <limits.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
struct ImageResult {
    std::string path;
    std::string name;
    std::string true_class;
    std::string predicted_class;
    float confidence = 0.0f;
};

struct DatasetTestState {
    bool running = false;
    bool finished = false;
    std::string mode = "dataset";
    std::string source_path;
    int processed = 0;
    int total = 0;
    int correct = 0;
    int load_failed = 0;
    double elapsed_ms = 0.0;
    std::string message = "尚未开始测试";
    std::vector<std::string> names;
    std::vector<int> class_total;
    std::vector<int> class_correct;
    std::vector<std::vector<int>> matrix;
    std::vector<ImageResult> results; // 全量模式仅错误项；文件夹模式为全部成功识别项
};

struct DatasetImage {
    std::string path;
    std::string name;
    int true_class;
};

DatasetTestState g_test_state;
std::mutex g_test_mutex;
std::atomic<bool> g_test_cancel(false);
std::mutex g_test_thread_mutex;
std::thread g_test_thread;

bool is_image_file(const std::string& name) {
    size_t dot = name.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = name.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return (char)tolower((unsigned char)c); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
}

std::vector<std::string> model_class_names() {
    std::vector<std::string> names;
    // ClassifyResult 当前最多保存 6 类概率；只使用核心模块公开 API。
    for (int i = 0; i < 6; ++i) {
        const char* name = class_name(i);
        if (!name || std::strcmp(name, "unknown") == 0) break;
        names.push_back(name);
    }
    return names;
}

bool canonical_directory(const char* raw, std::string& output) {
    if (!raw || !*raw) return false;
    char resolved[PATH_MAX];
    if (!realpath(raw, resolved)) return false;
    struct stat st;
    if (stat(resolved, &st) != 0 || !S_ISDIR(st.st_mode)) return false;
    output = resolved;
    return true;
}

std::string base_name(const std::string& path) {
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

void scan_images_recursive(const std::string& root, std::vector<std::string>& files, int depth = 0) {
    if (depth > 16) return;
    DIR* dir = opendir(root.c_str());
    if (!dir) return;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string path = root + "/" + name;
        struct stat st;
        if (lstat(path.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) scan_images_recursive(path, files, depth + 1);
        else if (S_ISREG(st.st_mode) && is_image_file(name)) files.push_back(path);
    }
    closedir(dir);
}

std::string safe_component(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (isalnum((unsigned char)c) || c == '_' || c == '-') out.push_back(c);
        else out.push_back('_');
    }
    return out.empty() ? "unknown" : out;
}

std::string safe_image_filename(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.') out.push_back(c);
        else out.push_back('_');
    }
    while (out.find("..") != std::string::npos) out.replace(out.find(".."), 2, "__");
    return out.empty() ? "image.png" : out;
}

void make_dir(const std::string& path) {
    if (mkdir(path.c_str(), 0755) != 0 && errno != EEXIST)
        fprintf(stderr, "[test] mkdir failed: %s\n", path.c_str());
}

void clear_directory_tree(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) return;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string child = path + "/" + name;
        struct stat st;
        if (lstat(child.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) { clear_directory_tree(child); rmdir(child.c_str()); }
        else unlink(child.c_str());
    }
    closedir(dir);
}

bool copy_file(const std::string& src, const std::string& dst) {
    FILE* in = fopen(src.c_str(), "rb");
    if (!in) return false;
    FILE* out = fopen(dst.c_str(), "wb");
    if (!out) { fclose(in); return false; }
    char buffer[16384];
    size_t n;
    bool ok = true;
    while ((n = fread(buffer, 1, sizeof(buffer), in)) > 0)
        if (fwrite(buffer, 1, n, out) != n) { ok = false; break; }
    if (ferror(in)) ok = false;
    fclose(in); fclose(out);
    return ok;
}

std::string json_escape(const std::string& value) {
    std::string out;
    for (unsigned char c : value) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else if (c != '\r' && c >= 0x20) out += (char)c;
    }
    return out;
}

bool test_cancelled() {
    return g_test_cancel.load();
}

void mark_cancelled() {
    std::lock_guard<std::mutex> lock(g_test_mutex);
    g_test_state.running = false;
    g_test_state.finished = true;
    g_test_state.message = "测试已取消";
}

void fail_task(const std::string& mode, const std::string& path, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_test_mutex);
    // 无效的新请求不得覆盖当前正在运行任务的进度和结果。
    if (g_test_state.running) return;
    g_test_state = DatasetTestState();
    g_test_state.mode = mode;
    g_test_state.source_path = path;
    g_test_state.finished = true;
    g_test_state.message = message;
}

void run_dataset_test(std::string root) {
    std::vector<std::string> names = model_class_names();
    if (names.empty()) { fail_task("dataset", root, "模型未加载或没有类别配置"); return; }

    std::vector<DatasetImage> files;
    for (size_t class_id = 0; class_id < names.size(); ++class_id) {
        std::string class_dir = root + "/" + names[class_id];
        DIR* dir = opendir(class_dir.c_str());
        if (!dir) continue;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (is_image_file(name)) files.push_back({class_dir + "/" + name, name, (int)class_id});
        }
        closedir(dir);
    }
    std::sort(files.begin(), files.end(), [](const DatasetImage& a, const DatasetImage& b) {
        return a.true_class != b.true_class ? a.true_class < b.true_class : a.name < b.name;
    });

    const std::string error_root = root + "/error";
    make_dir(error_root);
    clear_directory_tree(error_root);
    for (const auto& name : names) make_dir(error_root + "/" + safe_component(name));

    {
        std::lock_guard<std::mutex> lock(g_test_mutex);
        g_test_state = DatasetTestState();
        g_test_state.running = true;
        g_test_state.mode = "dataset";
        g_test_state.source_path = root;
        g_test_state.total = (int)files.size();
        g_test_state.names = names;
        g_test_state.class_total.assign(names.size(), 0);
        g_test_state.class_correct.assign(names.size(), 0);
        g_test_state.matrix.assign(names.size(), std::vector<int>(names.size(), 0));
        g_test_state.message = files.empty() ? "未在类别子目录中找到图片" : "正在进行全量测试";
    }

    auto begin = std::chrono::steady_clock::now();
    for (const auto& file : files) {
        if (test_cancelled()) { mark_cancelled(); return; }
        cv::Mat image = cv::imread(file.path, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::lock_guard<std::mutex> lock(g_test_mutex);
            ++g_test_state.load_failed; ++g_test_state.processed;
            continue;
        }
        ClassifyResult prediction = model_identify(image);
        bool valid = prediction.sort >= 0 && prediction.sort < (int)names.size();
        bool correct = valid && prediction.sort == file.true_class;
        std::string predicted = valid ? names[prediction.sort] : "unknown";
        {
            std::lock_guard<std::mutex> lock(g_test_mutex);
            ++g_test_state.class_total[file.true_class];
            if (valid) ++g_test_state.matrix[file.true_class][prediction.sort];
            if (correct) { ++g_test_state.correct; ++g_test_state.class_correct[file.true_class]; }
            else g_test_state.results.push_back({file.path, file.name, names[file.true_class], predicted, prediction.conf});
            ++g_test_state.processed;
            g_test_state.elapsed_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
        }
        if (!correct) {
            std::string predicted_dir = safe_component(predicted);
            make_dir(error_root + "/" + predicted_dir);
            copy_file(file.path, error_root + "/" + predicted_dir + "/" + safe_component(names[file.true_class]) + "__" + safe_image_filename(file.name));
        }
    }
    {
        std::lock_guard<std::mutex> lock(g_test_mutex);
        g_test_state.running = false; g_test_state.finished = true;
        g_test_state.elapsed_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
        g_test_state.message = files.empty() ? "没有可测试图片" : "全量测试完成";
    }
}

void run_folder_identification(std::string root) {
    std::vector<std::string> names = model_class_names();
    if (names.empty()) { fail_task("folder", root, "模型未加载或没有类别配置"); return; }
    std::vector<std::string> files;
    scan_images_recursive(root, files);
    std::sort(files.begin(), files.end());
    {
        std::lock_guard<std::mutex> lock(g_test_mutex);
        g_test_state = DatasetTestState();
        g_test_state.running = true;
        g_test_state.mode = "folder";
        g_test_state.source_path = root;
        g_test_state.total = (int)files.size();
        g_test_state.names = names;
        g_test_state.message = files.empty() ? "文件夹中没有可识别图片" : "正在识别文件夹图片";
    }
    auto begin = std::chrono::steady_clock::now();
    for (const auto& path : files) {
        if (test_cancelled()) { mark_cancelled(); return; }
        cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::lock_guard<std::mutex> lock(g_test_mutex);
            ++g_test_state.load_failed; ++g_test_state.processed;
            continue;
        }
        ClassifyResult prediction = model_identify(image);
        bool valid = prediction.sort >= 0 && prediction.sort < (int)names.size();
        std::string predicted = valid ? names[prediction.sort] : "unknown";
        {
            std::lock_guard<std::mutex> lock(g_test_mutex);
            g_test_state.results.push_back({path, base_name(path), "", predicted, prediction.conf});
            ++g_test_state.processed;
            g_test_state.elapsed_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
        }
    }
    {
        std::lock_guard<std::mutex> lock(g_test_mutex);
        g_test_state.running = false; g_test_state.finished = true;
        g_test_state.elapsed_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
        g_test_state.message = files.empty() ? "文件夹中没有可识别图片" : "文件夹识别完成";
    }
}

bool safe_model_name(const std::string& name) {
    if (name.empty() || name == "." || name == "..") return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '-' || c == '.';
    });
}

std::vector<std::string> available_models() {
    std::vector<std::string> models;
    std::string root = CAPTURE_MODEL_DIR;
    DIR* dir = opendir(root.c_str());
    if (!dir) return models;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (!safe_model_name(name)) continue;
        std::string folder = root + (root.back() == '/' ? "" : "/") + name;
        struct stat st;
        if (stat(folder.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        std::ifstream config(folder + "/" + name + ".json");
        if (!config.good()) continue;
        try {
            nlohmann::json j; config >> j;
            std::string param = j.value("param_file", "");
            std::string bin = j.value("bin_file", "");
            if (param.empty() || bin.empty() || param.find('/') != std::string::npos || bin.find('/') != std::string::npos) continue;
            if (access((folder + "/" + param).c_str(), R_OK) == 0 && access((folder + "/" + bin).c_str(), R_OK) == 0)
                models.push_back(name);
        } catch (...) {}
    }
    closedir(dir);
    std::sort(models.begin(), models.end());
    return models;
}

bool reserve_task(const std::string& mode, const std::string& path, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_test_mutex);
    if (g_test_state.running) return false;
    g_test_state = DatasetTestState();
    g_test_state.running = true;
    g_test_state.mode = mode;
    g_test_state.source_path = path;
    g_test_state.message = message;
    return true;
}
} // namespace

bool start_dataset_test(const char* dir_path) {
    std::string root;
    if (!canonical_directory((dir_path && *dir_path) ? dir_path : "/home/root/image/dataset", root)) {
        fail_task("dataset", dir_path ? dir_path : "", "数据集目录不存在或不可访问");
        return false;
    }
    {
        std::lock_guard<std::mutex> thread_lock(g_test_thread_mutex);
        if (g_test_thread.joinable()) g_test_thread.join();
    }
    if (!reserve_task("dataset", root, "正在扫描数据集")) return false;
    g_test_cancel.store(false);
    try {
        std::lock_guard<std::mutex> thread_lock(g_test_thread_mutex);
        g_test_thread = std::thread([root]() { run_dataset_test(root); });
    } catch (...) { fail_task("dataset", root, "无法启动测试线程"); return false; }
    return true;
}

bool start_folder_identification(const char* dir_path) {
    std::string root;
    if (!canonical_directory(dir_path, root)) {
        fail_task("folder", dir_path ? dir_path : "", "文件夹不存在或不可访问");
        return false;
    }
    {
        std::lock_guard<std::mutex> thread_lock(g_test_thread_mutex);
        if (g_test_thread.joinable()) g_test_thread.join();
    }
    if (!reserve_task("folder", root, "正在扫描文件夹")) return false;
    g_test_cancel.store(false);
    try {
        std::lock_guard<std::mutex> thread_lock(g_test_thread_mutex);
        g_test_thread = std::thread([root]() { run_folder_identification(root); });
    } catch (...) { fail_task("folder", root, "无法启动识别线程"); return false; }
    return true;
}

std::string dataset_test_status_json() {
    std::lock_guard<std::mutex> lock(g_test_mutex);
    const DatasetTestState& s = g_test_state;
    std::string json = "{";
    json += "\"running\":" + std::string(s.running ? "true" : "false") + ",";
    json += "\"finished\":" + std::string(s.finished ? "true" : "false") + ",";
    json += "\"mode\":\"" + json_escape(s.mode) + "\",";
    json += "\"source_path\":\"" + json_escape(s.source_path) + "\",";
    json += "\"processed\":" + std::to_string(s.processed) + ",\"total\":" + std::to_string(s.total) + ",";
    json += "\"correct\":" + std::to_string(s.correct) + ",";
    json += "\"error\":" + std::to_string(std::max(0, s.processed - s.correct - s.load_failed)) + ",";
    json += "\"load_failed\":" + std::to_string(s.load_failed) + ",";
    json += "\"elapsed_ms\":" + std::to_string(s.elapsed_ms) + ",";
    json += "\"message\":\"" + json_escape(s.message) + "\",\"classes\":[";
    for (size_t i = 0; i < s.names.size(); ++i) {
        if (i) json += ",";
        int total = i < s.class_total.size() ? s.class_total[i] : 0;
        int correct = i < s.class_correct.size() ? s.class_correct[i] : 0;
        json += "{\"name\":\"" + json_escape(s.names[i]) + "\",\"total\":" + std::to_string(total);
        json += ",\"correct\":" + std::to_string(correct) + ",\"error\":" + std::to_string(total - correct) + ",\"predictions\":[";
        for (size_t j = 0; j < s.names.size(); ++j) {
            if (j) json += ",";
            int value = i < s.matrix.size() && j < s.matrix[i].size() ? s.matrix[i][j] : 0;
            json += std::to_string(value);
        }
        json += "]}";
    }
    json += "],\"results\":[";
    for (size_t i = 0; i < s.results.size(); ++i) {
        if (i) json += ",";
        const ImageResult& r = s.results[i];
        json += "{\"id\":" + std::to_string(i) + ",\"name\":\"" + json_escape(r.name) + "\",";
        json += "\"true_class\":\"" + json_escape(r.true_class) + "\",\"predicted_class\":\"" + json_escape(r.predicted_class) + "\",";
        json += "\"confidence\":" + std::to_string(r.confidence) + "}";
    }
    json += "]}";
    return json;
}

bool dataset_test_running() {
    std::lock_guard<std::mutex> lock(g_test_mutex);
    return g_test_state.running;
}

void request_stop_dataset_test() {
    g_test_cancel.store(true);
}

void stop_dataset_test() {
    g_test_cancel.store(true);
    std::lock_guard<std::mutex> thread_lock(g_test_thread_mutex);
    if (g_test_thread.joinable()) g_test_thread.join();
    if (dataset_test_running()) mark_cancelled();
}

std::string model_catalog_json() {
    std::vector<std::string> models = available_models();
    std::string current = current_model_name();
    std::string json = "{\"current\":\"" + json_escape(current) + "\",\"models\":[";
    for (size_t i = 0; i < models.size(); ++i) {
        if (i) json += ",";
        json += "\"" + json_escape(models[i]) + "\"";
    }
    json += "]}";
    return json;
}

bool switch_test_model(const char* raw_name, std::string& message) {
    std::string name = raw_name ? raw_name : "";
    if (dataset_test_running()) { message = "测试运行中，不能切换模型"; return false; }
    std::vector<std::string> models = available_models();
    if (std::find(models.begin(), models.end(), name) == models.end()) {
        message = "模型不存在或文件不完整";
        return false;
    }
    if (name == current_model_name()) { message = "当前已经是该模型"; return true; }
    // 切换模型必须先释放当前模型资源，再加载新模型。
    model_unload();
    if (!model_load(name.c_str())) { message = "旧模型已卸载，但新模型加载失败"; return false; }
    message = "已切换到模型 " + name;
    return true;
}

bool dataset_test_result_image(int result_id, std::vector<uchar>& encoded_jpeg) {
    std::string path;
    {
        std::lock_guard<std::mutex> lock(g_test_mutex);
        if (result_id < 0 || result_id >= (int)g_test_state.results.size()) return false;
        path = g_test_state.results[result_id].path;
    }
    cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
    return !image.empty() && cv::imencode(".jpg", image, encoded_jpeg);
}
