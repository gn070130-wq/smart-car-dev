#include "zf_common_headfile.h"
#include "web_service_main.h"
#include "web_pages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include "classifier.h"
#include "dataset_test_service.h"
#include "my_timeer.h"

float used_time = 0.0;

extern int ttt;
extern int *key_scan;
extern int key_scan_gpio;
extern int fd_keyboard;

extern int accel_num[];
extern int gyro_num[];
extern int left_corner[100][2];
extern int right_corner[100][2];
extern uint8_t *PerImg_ip[240][320];
extern int test_bianliang;
extern int left_straight;
extern int right_straight;
extern Mat grayImage;
extern Mat square;
extern int mid_x;
extern int mid_y;
extern int left_lenth;
extern int mid_lenth_left;
extern int right_lenth;
extern int left_start;
extern int right_start;
extern int left_line_raw[1000][2];
extern int mid_line_raw_from_left[1000][2];
extern int mid_line_raw_from_right[1000][2];
extern int right_line_raw[1000][2];
extern int pwm_fan;
extern int left_raw_lose;
extern int right_raw_lose;
extern int mid_lose;
extern int pwm_servo;
extern int pwm_servo_true;
extern float dif_speed;
extern int mid_straight;
extern int barrier_point[2];
extern int barrier_flag;
extern int barrier_save;
extern float error_val;
extern ClassifyResult result;

// ==================== 网页采集服务配置 ====================
#define CAPTURE_DEFAULT_PREFIX "bomb"
#define CAPTURE_SAVE_DIR       "/home/root/image/"
#define CAPTURE_CONFIG_PATH    CAPTURE_SAVE_DIR "capture.conf"
#define CAPTURE_HTTP_PORT      8080

extern Mat square;
extern void cut_image(int y_center, int x_center, int length);
extern void print_image(cv::Mat &frame, int y, int x);
extern int keyboard_scan_gpio();
extern void print_pixel(int y, int x, const char* color);

static std::mutex g_capture_mutex;
static std::atomic<bool> g_capture_running(false);
static std::atomic<bool> g_capture_processing(false);
static std::atomic<long long> g_camera_preview_until_ms(0);
static std::string g_capture_prefix = CAPTURE_DEFAULT_PREFIX;
static std::string g_capture_dir = CAPTURE_SAVE_DIR;
static int g_img_seq = 0;
static int g_line_chose_without_circle = -1;
static std::string g_last_message = "ready";
static std::string g_last_classify = "not classified";

static std::string json_escape(const std::string &s);

static long long steady_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static bool camera_preview_active() {
    return g_camera_preview_until_ms.load() > steady_ms();
}

static void keep_camera_preview_alive() {
    g_camera_preview_until_ms.store(steady_ms() + 3000);
}

static std::string camera_config_json(const std::string &message = "") {
    CameraExposureConfig cfg;
    bool ok = camera_exposure_get(cfg);
    std::string json = "{\"success\":" + std::string(ok ? "true" : "false");
    json += ",\"preview\":" + std::string(camera_preview_active() ? "true" : "false");
    json += ",\"auto_exposure\":" + std::string(cfg.auto_exposure ? "true" : "false");
    json += ",\"exposure_time\":" + std::to_string(cfg.exposure_time);
    json += ",\"exposure_min\":" + std::to_string(cfg.exposure_min);
    json += ",\"exposure_max\":" + std::to_string(cfg.exposure_max);
    json += ",\"exposure_step\":" + std::to_string(cfg.exposure_step);
    json += ",\"auto_priority\":" + std::string(cfg.auto_priority ? "true" : "false");
    json += ",\"auto_priority_available\":" + std::string(cfg.auto_priority_available ? "true" : "false");
    json += ",\"message\":\"" + json_escape(message.empty() ? (ok ? "摄像头参数已读取" : "摄像头曝光控制不可用") : message) + "\"}";
    return json;
}

static void append_camera_control_json(std::string &json, const std::string &name,
                                       const CameraControlValue &c) {
    json += ",\"" + name + "\":{";
    json += "\"available\":" + std::string(c.available ? "true" : "false");
    json += ",\"inactive\":" + std::string(c.inactive ? "true" : "false");
    json += ",\"value\":" + std::to_string(c.value);
    json += ",\"min\":" + std::to_string(c.minimum);
    json += ",\"max\":" + std::to_string(c.maximum);
    json += ",\"step\":" + std::to_string(c.step) + "}";
}

static std::string camera_image_controls_json(const std::string &message = "") {
    CameraImageControls controls;
    bool ok = camera_image_controls_get(controls);
    std::string json = "{\"success\":" + std::string(ok ? "true" : "false");
    append_camera_control_json(json, "brightness", controls.brightness);
    append_camera_control_json(json, "contrast", controls.contrast);
    append_camera_control_json(json, "saturation", controls.saturation);
    append_camera_control_json(json, "hue", controls.hue);
    append_camera_control_json(json, "auto_white_balance", controls.auto_white_balance);
    append_camera_control_json(json, "gamma", controls.gamma);
    append_camera_control_json(json, "power_line_frequency", controls.power_line_frequency);
    append_camera_control_json(json, "white_balance_temperature", controls.white_balance_temperature);
    append_camera_control_json(json, "sharpness", controls.sharpness);
    json += ",\"message\":\"" + json_escape(message.empty() ?
        (ok ? "画面参数已读取" : "摄像头画面参数不可用") : message) + "\"}";
    return json;
}

static bool has_suffix(const std::string &s, const std::string &suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string json_escape(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': break;
        default: out += c; break;
        }
    }
    return out;
}

static std::string url_decode(const std::string &s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() && isxdigit((unsigned char)s[i + 1]) && isxdigit((unsigned char)s[i + 2])) {
            char hex[3] = {s[i + 1], s[i + 2], 0};
            out.push_back((char)strtol(hex, NULL, 16));
            i += 2;
        } else if (s[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

static std::string get_param(const std::string &data, const std::string &key) {
    size_t pos = 0;
    while (pos < data.size()) {
        size_t amp = data.find('&', pos);
        std::string part = data.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
        size_t eq = part.find('=');
        std::string k = url_decode(eq == std::string::npos ? part : part.substr(0, eq));
        if (k == key) return url_decode(eq == std::string::npos ? "" : part.substr(eq + 1));
        if (amp == std::string::npos) break;
        pos = amp + 1;
    }
    return "";
}

static std::string sanitize_prefix(const std::string &raw) {
    std::string out;
    for (char c : raw) {
        if (isalnum((unsigned char)c) || c == '_' || c == '-') out.push_back(c);
        if (out.size() >= 32) break;
    }
    return out.empty() ? CAPTURE_DEFAULT_PREFIX : out;
}

static bool normalize_capture_dir(const std::string &raw, std::string &out) {
    std::string path = raw;
    while (!path.empty() && isspace((unsigned char)path.front())) path.erase(path.begin());
    while (!path.empty() && isspace((unsigned char)path.back())) path.pop_back();
    if (path.empty()) path = CAPTURE_SAVE_DIR;
    if (path.front() != '/') path = std::string(CAPTURE_SAVE_DIR) + path;
    std::vector<std::string> parts; size_t pos = 0;
    while (pos <= path.size()) {
        size_t slash = path.find('/', pos);
        std::string part = path.substr(pos, slash == std::string::npos ? std::string::npos : slash - pos);
        if (!part.empty() && part != ".") {
            if (part == "..") { if (parts.empty()) return false; parts.pop_back(); }
            else parts.push_back(part);
        }
        if (slash == std::string::npos) break;
        pos = slash + 1;
    }
    out = "/";
    for (const auto &part : parts) out += part + "/";
    const std::string root = CAPTURE_SAVE_DIR;
    return out.size() >= root.size() && out.compare(0, root.size(), root) == 0;
}

static bool ensure_directory(const std::string &path) {
    std::string current;
    for (char c : path) {
        current.push_back(c);
        if (c == '/' && current.size() > 1 && mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) return false;
    }
    return true;
}

static void load_capture_config() {
    g_capture_prefix = CAPTURE_DEFAULT_PREFIX; g_capture_dir = CAPTURE_SAVE_DIR;
    FILE *fp = fopen(CAPTURE_CONFIG_PATH, "r"); if (!fp) return;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        std::string value = line; while (!value.empty() && isspace((unsigned char)value.back())) value.pop_back();
        if (value.compare(0, 7, "prefix=") == 0) g_capture_prefix = sanitize_prefix(value.substr(7));
        else if (value.compare(0, 4, "dir=") == 0) { std::string dir; if (normalize_capture_dir(value.substr(4), dir)) g_capture_dir = dir; }
    }
    fclose(fp); ensure_directory(g_capture_dir);
}

static bool save_capture_config() {
    FILE *fp = fopen(CAPTURE_CONFIG_PATH, "w"); if (!fp) return false;
    fprintf(fp, "prefix=%s\ndir=%s\n", sanitize_prefix(g_capture_prefix).c_str(), g_capture_dir.c_str());
    fclose(fp); return true;
}

static bool safe_image_name(const std::string &name) {
    if (name.empty() || !has_suffix(name, ".png")) return false;
    for (char c : name) {
        if (!(isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.')) return false;
    }
    return name.find("..") == std::string::npos;
}

static std::vector<std::string> list_images() {
    struct ImageItem {
        std::string name;
        time_t mtime;
    };
    std::vector<ImageItem> items;
    DIR *dir = opendir(g_capture_dir.c_str());
    if (!dir) return {};
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (!safe_image_name(name)) continue;
        struct stat st;
        std::string full = g_capture_dir + name;
        time_t mtime = (stat(full.c_str(), &st) == 0) ? st.st_mtime : 0;
        items.push_back({name, mtime});
    }
    closedir(dir);
    std::sort(items.begin(), items.end(), [](const ImageItem &a, const ImageItem &b) {
        if (a.mtime != b.mtime) return a.mtime > b.mtime;
        return a.name > b.name;
    });
    std::vector<std::string> images;
    images.reserve(items.size());
    for (const auto &item : items) images.push_back(item.name);
    return images;
}

static int get_next_sub_seq(const std::string &prefix_name, int img_seq) {
    int max_sub_seq = 0;
    std::string prefix = prefix_name + "_" + std::to_string(img_seq) + "_";
    DIR *dir = opendir(g_capture_dir.c_str());
    if (!dir) return 1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name.size() > prefix.size() + 4 && name.compare(0, prefix.size(), prefix) == 0 && has_suffix(name, ".png")) {
            std::string sub = name.substr(prefix.size(), name.size() - prefix.size() - 4);
            bool number = !sub.empty() && std::all_of(sub.begin(), sub.end(), [](char c){ return isdigit((unsigned char)c); });
            if (number) max_sub_seq = std::max(max_sub_seq, atoi(sub.c_str()));
        }
    }
    closedir(dir);
    return max_sub_seq + 1;
}

static bool send_all(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    while (len > 0) {
        ssize_t n = send(fd, p, len, 0);
        if (n <= 0) return false;
        p += n;
        len -= (size_t)n;
    }
    return true;
}

static void http_send(int fd, const std::string &status, const std::string &type, const std::string &body) {
    std::string header = "HTTP/1.1 " + status + "\r\nContent-Type: " + type + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n";
    send_all(fd, header.data(), header.size());
    send_all(fd, body.data(), body.size());
}

static void http_send_binary(int fd, const std::string &type, const std::vector<uchar> &body) {
    std::string header = "HTTP/1.1 200 OK\r\nContent-Type: " + type + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n";
    send_all(fd, header.data(), header.size());
    if (!body.empty()) send_all(fd, body.data(), body.size());
}

static void http_send_file(int fd, const std::string &path, const std::string &type) {
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        http_send(fd, "404 Not Found", "text/plain; charset=utf-8", "not found");
        return;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::string header = "HTTP/1.1 200 OK\r\nContent-Type: " + type + "\r\nContent-Length: " + std::to_string(size) + "\r\nConnection: close\r\n\r\n";
    send_all(fd, header.data(), header.size());
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) send_all(fd, buf, n);
    fclose(fp);
}

static std::string status_json() {
    std::lock_guard<std::mutex> lock(g_capture_mutex);
    std::vector<std::string> images = list_images();
    std::string json = "{";
    json += "\"prefix\":\"" + json_escape(g_capture_prefix) + "\",";
    json += "\"capture_dir\":\"" + json_escape(g_capture_dir) + "\",";
    json += "\"capture_processing\":" + std::string(g_capture_processing ? "true" : "false") + ",";
    json += "\"img_seq\":" + std::to_string(g_img_seq) + ",";
    json += "\"barrier_flag\":" + std::to_string(barrier_flag) + ",";
    json += "\"line_chose_without_circle\":" + std::to_string(g_line_chose_without_circle) + ",";
    json += "\"battery\":\"" + json_escape(std::to_string(get_battery_voltage())) + "\",";
    json += "\"message\":\"" + json_escape(g_last_message) + "\",";
    json += "\"classify\":\"" + json_escape(g_last_classify) + "\",";
    json += "\"images\":[";
    for (size_t i = 0; i < images.size(); ++i) {
        if (i) json += ",";
        json += "\"" + json_escape(images[i]) + "\"";
    }
    json += "]}";
    return json;
}

static void handle_client(int fd) {
    std::string req;
    char buf[4096];
    ssize_t n;
    while ((n = recv(fd, buf, sizeof(buf), 0)) > 0) {
        req.append(buf, n);
        if (req.find("\r\n\r\n") != std::string::npos) break;
        if (req.size() > 65536) break;
    }
    if (req.empty()) { close(fd); return; }

    size_t line_end = req.find("\r\n");
    std::string first = req.substr(0, line_end);
    size_t sp1 = first.find(' '), sp2 = first.find(' ', sp1 + 1);
    std::string method = sp1 == std::string::npos ? "" : first.substr(0, sp1);
    std::string uri = (sp1 == std::string::npos || sp2 == std::string::npos) ? "/" : first.substr(sp1 + 1, sp2 - sp1 - 1);
    std::string path = uri, query;
    size_t q = uri.find('?');
    if (q != std::string::npos) { path = uri.substr(0, q); query = uri.substr(q + 1); }

    size_t header_end = req.find("\r\n\r\n");
    size_t content_length = 0;
    size_t cl = req.find("Content-Length:");
    if (cl != std::string::npos) content_length = (size_t)atoi(req.c_str() + cl + 15);
    std::string body = header_end == std::string::npos ? "" : req.substr(header_end + 4);
    while (body.size() < content_length && (n = recv(fd, buf, sizeof(buf), 0)) > 0) body.append(buf, n);
    if (body.size() > content_length) body.resize(content_length);

    if (method == "GET" && path == "/") {
        http_send(fd, "200 OK", "text/html; charset=utf-8", web_pages::home_html());
    } else if (method == "GET" && (path == "/dataset-capture" || path == "/capture")) {
        http_send(fd, "200 OK", "text/html; charset=utf-8", web_pages::capture_html());
    } else if (method == "GET" && path == "/model-test") {
        http_send(fd, "200 OK", "text/html; charset=utf-8", web_pages::model_test_html());
    } else if (method == "GET" && path == "/camera-settings") {
        http_send(fd, "200 OK", "text/html; charset=utf-8", web_pages::camera_settings_html());
    } else if (method == "POST" && path == "/api/camera/preview/start") {
        if (g_capture_processing) {
            http_send(fd, "409 Conflict", "application/json; charset=utf-8", camera_config_json("数据集采集中，无法启动摄像头参数预览"));
        } else {
            keep_camera_preview_alive();
            http_send(fd, "200 OK", "application/json; charset=utf-8", camera_config_json());
        }
    } else if (method == "POST" && path == "/api/camera/preview/stop") {
        g_camera_preview_until_ms.store(0);
        http_send(fd, "200 OK", "application/json; charset=utf-8", camera_config_json("实时预览已停止"));
    } else if (method == "GET" && path == "/api/camera/frame") {
        if (g_capture_processing) {
            g_camera_preview_until_ms.store(0);
            http_send(fd, "409 Conflict", "text/plain; charset=utf-8", "dataset capture is running");
        } else {
            keep_camera_preview_alive();
            std::vector<uchar> data;
            {
                std::lock_guard<std::mutex> lock(g_capture_mutex);
                if (!g_yuyv.empty()) {
                    cv::Mat bgr;
                    cv::cvtColor(g_yuyv, bgr, cv::COLOR_YUV2BGR_YUY2);
                    cv::imencode(".jpg", bgr, data);
                }
            }
            if (data.empty()) http_send(fd, "503 Service Unavailable", "text/plain; charset=utf-8", "no camera frame");
            else http_send_binary(fd, "image/jpeg", data);
        }
    } else if (method == "GET" && path == "/api/camera/image-settings") {
        http_send(fd, "200 OK", "application/json; charset=utf-8", camera_image_controls_json());
    } else if (method == "POST" && path == "/api/camera/image-settings") {
        CameraImageControls requested;
        camera_image_controls_get(requested);
        requested.brightness.value = atoi(get_param(body, "brightness").c_str());
        requested.contrast.value = atoi(get_param(body, "contrast").c_str());
        requested.saturation.value = atoi(get_param(body, "saturation").c_str());
        requested.hue.value = atoi(get_param(body, "hue").c_str());
        requested.auto_white_balance.value = atoi(get_param(body, "auto_white_balance").c_str());
        requested.gamma.value = atoi(get_param(body, "gamma").c_str());
        requested.power_line_frequency.value = atoi(get_param(body, "power_line_frequency").c_str());
        requested.white_balance_temperature.value = atoi(get_param(body, "white_balance_temperature").c_str());
        requested.sharpness.value = atoi(get_param(body, "sharpness").c_str());
        CameraImageControls applied; std::string message; bool ok;
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            ok = camera_image_controls_set(requested, applied, message);
        }
        http_send(fd, ok ? "200 OK" : "400 Bad Request", "application/json; charset=utf-8",
                  camera_image_controls_json(message));
    } else if (method == "GET" && path == "/api/camera/settings") {
        http_send(fd, "200 OK", "application/json; charset=utf-8", camera_config_json());
    } else if (method == "POST" && path == "/api/camera/settings") {
        bool auto_mode = get_param(body, "auto") == "true";
        bool priority = get_param(body, "priority") == "true";
        int exposure_time = atoi(get_param(body, "exposure").c_str());
        CameraExposureConfig applied; std::string message;
        bool ok;
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            ok = camera_exposure_set(auto_mode, exposure_time, priority, applied, message);
        }
        std::string response = camera_config_json(message);
        http_send(fd, ok ? "200 OK" : "400 Bad Request", "application/json; charset=utf-8", response);
    } else if (method == "GET" && path == "/api/status") {
        http_send(fd, "200 OK", "application/json; charset=utf-8", status_json());
    } else if (method == "GET" && path == "/api/frame") {
        std::vector<uchar> data;
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            if (!square.empty()) imencode(".jpg", square, data);
        }
        if (data.empty()) http_send(fd, "503 Service Unavailable", "text/plain; charset=utf-8", "no frame");
        else http_send_binary(fd, "image/jpeg", data);
    } else if (method == "GET" && path == "/image") {
        std::string name = get_param(query, "name");
        if (!safe_image_name(name)) http_send(fd, "400 Bad Request", "text/plain; charset=utf-8", "bad name");
        else http_send_file(fd, std::string(CAPTURE_SAVE_DIR) + name, "image/png");
    } else if (method == "POST" && path == "/api/prefix") {
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            g_capture_prefix = sanitize_prefix(get_param(body, "prefix"));
            bool ok = save_capture_config();
            g_last_message = ok ? ("prefix set to " + g_capture_prefix) : ("prefix set but config save failed: " + g_capture_prefix);
        }
        http_send(fd, "200 OK", "application/json; charset=utf-8", status_json());
    } else if (method == "POST" && path == "/api/capture-dir") {
        std::string dir;
        bool valid = normalize_capture_dir(get_param(body, "dir"), dir);
        bool created = valid && ensure_directory(dir);
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            if (created) {
                g_capture_dir = dir;
                bool saved = save_capture_config();
                g_last_message = saved ? ("保存目录已切换为 " + dir) : ("目录已切换，但配置保存失败：" + dir);
            } else {
                g_last_message = valid ? ("无法创建目录：" + dir) : "保存目录必须位于 /home/root/image/ 下";
            }
        }
        http_send(fd, created ? "200 OK" : "400 Bad Request", "application/json; charset=utf-8", status_json());
    } else if (method == "POST" && path == "/api/seq") {
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            int delta = atoi(get_param(body, "delta").c_str());
            int seq = atoi(get_param(body, "seq").c_str());
            if (body.find("seq=") != std::string::npos) g_img_seq = std::max(0, seq);
            else g_img_seq = std::max(0, g_img_seq + delta);
            g_last_message = "image sequence: " + std::to_string(g_img_seq);
        }
        http_send(fd, "200 OK", "application/json; charset=utf-8", status_json());
    } else if (method == "POST" && path == "/api/capture") {
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            if (barrier_flag < 0 || square.empty()) {
                g_last_message = "No barrier detected, cannot capture.";
            } else {
                int sub_seq = get_next_sub_seq(g_capture_prefix, g_img_seq);
                std::string file = g_capture_prefix + "_" + std::to_string(g_img_seq) + "_" + std::to_string(sub_seq) + ".png";
                std::string full = g_capture_dir + file;
                g_last_message = imwrite(full, square) ? ("saved " + full) : ("save failed " + full);
            }
        }
        http_send(fd, "200 OK", "application/json; charset=utf-8", status_json());
    } else if (method == "POST" && path == "/api/delete") {
        std::string name = get_param(body, "name");
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            if (!safe_image_name(name)) g_last_message = "bad image name";
            else {
                std::string full = g_capture_dir + name;
                g_last_message = (unlink(full.c_str()) == 0) ? ("deleted " + name) : ("delete failed " + name);
            }
        }
        http_send(fd, "200 OK", "application/json; charset=utf-8", status_json());
    } else if (method == "POST" && path == "/api/classify") {
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            if (barrier_flag >= 0 && !square.empty()) {
                uint64_t start_time = get_wall_time_ns();
                result = model_identify(square);
                uint64_t end_time = get_wall_time_ns();
                used_time = (end_time - start_time) / 1e6;
                char tmp[512];
                snprintf(tmp, sizeof(tmp), "result: %s (id=%d)\naction: %s\nconfidence: %.2f%%\nprob: %.4f %.4f %.4f %.4f %.4f %.4f\nused_time: %.2f ms",
                         class_name(result.sort), result.sort, action_name(result.sort), result.conf * 100.0f,
                         result.prob[0], result.prob[1], result.prob[2], result.prob[3], result.prob[4], result.prob[5], used_time);
                g_last_classify = tmp;
                g_last_message = "classified";
            } else {
                g_last_message = "failed to classify image";
            }
        }
        http_send(fd, "200 OK", "application/json; charset=utf-8", status_json());
    } else if (method == "POST" && path == "/api/capture/start") {
        g_camera_preview_until_ms.store(0);
        g_capture_processing = true;
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            g_last_message = "数据集采集已开始";
        }
        http_send(fd, "200 OK", "application/json; charset=utf-8", status_json());
    } else if (method == "POST" && path == "/api/capture/stop") {
        g_capture_processing = false;
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            g_last_message = "数据集采集已停止";
        }
        http_send(fd, "200 OK", "application/json; charset=utf-8", status_json());
    } else if (method == "GET" && path == "/api/models") {
        http_send(fd, "200 OK", "application/json; charset=utf-8", model_catalog_json());
    } else if (method == "POST" && path == "/api/models/switch") {
        std::string message;
        bool switched = switch_test_model(get_param(body, "name").c_str(), message);
        std::string response = "{\"success\":" + std::string(switched ? "true" : "false") + ",\"message\":\"" + json_escape(message) + "\",";
        response += "\"catalog\":" + model_catalog_json() + "}";
        http_send(fd, "200 OK", "application/json; charset=utf-8", response);
    } else if (method == "GET" && path == "/api/test/result-image") {
        std::vector<uchar> data;
        std::string id_text = get_param(query, "id");
        bool valid_id = !id_text.empty() && std::all_of(id_text.begin(), id_text.end(), [](char c) { return isdigit((unsigned char)c); });
        int id = valid_id ? atoi(id_text.c_str()) : -1;
        if (valid_id && dataset_test_result_image(id, data)) http_send_binary(fd, "image/jpeg", data);
        else http_send(fd, "404 Not Found", "text/plain; charset=utf-8", "result image not found");
    } else if (method == "GET" && path == "/api/test/status") {
        http_send(fd, "200 OK", "application/json; charset=utf-8", dataset_test_status_json());
    } else if (method == "POST" && path == "/api/test/start") {
        bool started = start_dataset_test("/home/root/image/dataset");
        std::string state = dataset_test_status_json();
        std::string response = "{\"started\":" + std::string(started ? "true" : "false") + ",\"state\":" + state + "}";
        http_send(fd, "200 OK", "application/json; charset=utf-8", response);
    } else if (method == "POST" && path == "/api/test/folder/start") {
        std::string folder = get_param(body, "path");
        bool started = start_folder_identification(folder.c_str());
        std::string state = dataset_test_status_json();
        std::string response = "{\"started\":" + std::string(started ? "true" : "false") + ",\"state\":" + state + "}";
        http_send(fd, "200 OK", "application/json; charset=utf-8", response);
    } else if (method == "POST" && path == "/api/stop") {
        request_stop_dataset_test();
        g_camera_preview_until_ms.store(0);
        g_capture_processing = false;
        g_capture_running = false;
        http_send(fd, "200 OK", "application/json; charset=utf-8", "{\"ok\":true}");
    } else {
        http_send(fd, "404 Not Found", "text/plain; charset=utf-8", "not found");
    }
    close(fd);
}

static void http_server_thread() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        g_capture_running = false;
        return;
    }
    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(CAPTURE_HTTP_PORT);
    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0 || listen(server_fd, 8) < 0) {
        perror("bind/listen");
        close(server_fd);
        g_capture_running = false;
        return;
    }
    printf("Web service: http://0.0.0.0:%d/\n", CAPTURE_HTTP_PORT);

    while (g_capture_running) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(server_fd, &rfds);
        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int ret = select(server_fd + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(server_fd, &rfds)) {
            int client = accept(server_fd, NULL, NULL);
            if (client >= 0) handle_client(client);
        }
    }
    close(server_fd);
}

/**
 * @brief 网页服务主入口
 *
 * 启动纯 C/C++ HTTP 服务。浏览器访问 http://设备IP:8080/ 后先选择服务：
 * - 数据集拍摄：按需启动图像处理并查看 square
 * - 修改保存前缀
 * - LEFT/RIGHT 调整图片序号
 * - SAVE 保存图片，命名：前缀_图片序号_次序号.png
 * - CLASSIFY 对当前 square 分类
 * - 浏览、查看、删除已保存图片
 * - 模型识别测试：批量测试分类数据集并查看统计结果
 * - EXIT 退出整个网页服务
 */
extern int barrier_flag_count;

void web_service_menu() {
    barrier_flag = -2;
    barrier_save = -2;
    g_capture_running = true;
    g_capture_processing = false;
    g_camera_preview_until_ms.store(0);
    system("mkdir -p " CAPTURE_SAVE_DIR);
    load_capture_config();
    g_img_seq = 0;
    g_line_chose_without_circle = -1;
    g_last_message = "采集已停止，请点击开始采集";
    g_last_classify = "not classified";

    // 相机预取和透视初始化延迟到第一次点击“开始采集”，默认不访问采集流程。
    bool capture_initialized = false;

    std::thread server(http_server_thread);
    printf("Open browser: http://<device-ip>:%d/\n", CAPTURE_HTTP_PORT);

    while (g_capture_running) {
        if (!g_capture_processing) {
            if (camera_preview_active()) {
                {
                    std::lock_guard<std::mutex> lock(g_capture_mutex);
                    get_mmap_image(3);
                }
                msleep(15);
            } else {
                msleep(50);
            }
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(g_capture_mutex);
            if (!g_capture_processing) continue;
            if (!capture_initialized) {
                get_mmap_image(3);
                ImagePerspective1_Color_Init();
                ImagePerspective2_Color_Init();
                ImagePerspective3_Color_Init();
                capture_initialized = true;
            }
            barrier_flag = -3;
            get_mmap_image(1);
            line_init();
            find_edge_line();
            filter_lines(0, 13);
            filter_lines(1, 13);

            resample_track_line(0, 5);
            resample_track_line(1, 5);

            find_left_corner(0);
            find_right_corner(0);

            find_mid_line_from_left(0, 50);
            find_mid_line_from_right(0, 50);

            resample_track_line(2, 5);
            resample_track_line(3, 5);
            g_line_chose_without_circle = chose_line_without_circle(barrier_save);
            error_val = find_curve_all();
            barrier_flag = find_barrier_accurate(g_line_chose_without_circle);
            if(barrier_flag_count > 100){
                barrier_flag_count = 0;
            }
        }
        msleep(10);
    }

    // 退出前取消所有测试任务；不卸载模型，模型仍由主程序管理。
    request_stop_dataset_test();
    g_capture_processing = false;
    if (server.joinable()) server.join();
    stop_dataset_test();
    printf("\n[WEB SERVICE EXIT]\n");
}
