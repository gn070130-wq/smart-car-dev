#include "headfile.h"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <iostream>


static inline float intersection_area(const DetectObject& a, const DetectObject& b)
{
    cv::Rect_<float> inter = a.rect & b.rect;
    return inter.area();
}

static void qsort_descent_inplace(std::vector<DetectObject>& objs, int left, int right)
{
    int i = left;
    int j = right;
    float p = objs[(left + right) / 2].prob;

    while (i <= j) {
        while (objs[i].prob > p) i++;
        while (objs[j].prob < p) j--;
        if (i <= j) {
            std::swap(objs[i], objs[j]);
            i++;
            j--;
        }
    }
    if (left < j)  qsort_descent_inplace(objs, left, j);
    if (i < right)  qsort_descent_inplace(objs, i, right);
}

static void qsort_descent_inplace(std::vector<DetectObject>& objs)
{
    if (objs.empty()) return;
    qsort_descent_inplace(objs, 0, (int)objs.size() - 1);
}

static void nms_sorted_bboxes(const std::vector<DetectObject>& objs,
                               std::vector<int>& picked,
                               float nms_threshold)
{
    picked.clear();
    const int n = (int)objs.size();
    std::vector<float> areas(n);
    for (int i = 0; i < n; i++)
        areas[i] = objs[i].rect.area();

    for (int i = 0; i < n; i++) {
        const DetectObject& a = objs[i];
        int keep = 1;
        for (int j = 0; j < (int)picked.size(); j++) {
            const DetectObject& b = objs[picked[j]];
            if (a.label != b.label) continue;
            float inter = intersection_area(a, b);
            float uni   = areas[i] + areas[picked[j]] - inter;
            if (inter / uni > nms_threshold) keep = 0;
        }
        if (keep) picked.push_back(i);
    }
}

static inline float sigmoid(float x)
{
    return 1.0f / (1.0f + expf(-x));
}

static void generate_proposals(const ncnn::Mat& anchors, int stride,
                               const ncnn::Mat& in_pad,
                               const ncnn::Mat& feat_blob,
                               float prob_threshold,
                               std::vector<DetectObject>& objects)
{
    const int num_w = feat_blob.w;
    const int num_grid_y = feat_blob.c;
    const int num_grid_x = feat_blob.h;
    const int num_anchors = anchors.w / 2;
    const int walk = num_w / num_anchors;
    const int num_class = 3;

    for (int i = 0; i < num_grid_y; i++) {
        for (int j = 0; j < num_grid_x; j++) {
            const float* matat = feat_blob.channel(i).row(j);
            for (int k = 0; k < num_anchors; k++) {
                const float anchor_w = anchors[k * 2];
                const float anchor_h = anchors[k * 2 + 1];
                const float* ptr = matat + k * walk;
                float box_confidence = ptr[4];
                if (box_confidence >= prob_threshold) {
                    int class_index = 0;
                    float class_score = -FLT_MAX;
                    for (int c = 0; c < num_class; c++) {
                        float score = ptr[5 + c];
                        if (score > class_score) {
                            class_index = c;
                            class_score = score;
                        }
                    }
                    float confidence = box_confidence * class_score;
                    if (confidence >= prob_threshold) {
                        float dx = ptr[0];
                        float dy = ptr[1];
                        float dw = ptr[2];
                        float dh = ptr[3];

                        float pb_cx = (dx * 2.f - 0.5f + j) * stride;
                        float pb_cy = (dy * 2.f - 0.5f + i) * stride;
                        float pb_w  = powf(dw * 2.f, 2) * anchor_w;
                        float pb_h  = powf(dh * 2.f, 2) * anchor_h;

                        DetectObject obj;
                        obj.rect.x      = pb_cx - pb_w * 0.5f;
                        obj.rect.y      = pb_cy - pb_h * 0.5f;
                        obj.rect.width  = pb_w;
                        obj.rect.height = pb_h;
                        obj.label       = class_index;
                        obj.prob        = confidence;
                        objects.push_back(obj);
                    }
                }
            }
        }
    }
}

//YoloDetector 实现

YoloDetector::YoloDetector()
    : model_loaded(false)
    , target_size(640)
{
}

YoloDetector::~YoloDetector()
{
    
}


void YoloDetector::build_category_map()
{
    // 假设3个类别
    label_to_category.resize(3, TARGET_NONE);
    label_to_category[0] = TARGET_SUPPLY;
    label_to_category[1] = TARGET_VEHICLE;
    label_to_category[2] = TARGET_WEAPON;
}

int YoloDetector::load_model(const std::string& param_path,
                              const std::string& model_path)
{
    net.opt.use_vulkan_compute = false;  // 嵌入式板子通常不支持 Vulkan
    net.opt.num_threads = 2;             // 根据板子核心数调整

    if (net.load_param(param_path.c_str())) {
        std::cerr << "[YOLO] 加载 param 失败: " << param_path << "\n";
        return -1;
    }
    if (net.load_model(model_path.c_str())) {
        std::cerr << "[YOLO] 加载 model 失败: " << model_path << "\n";
        return -1;
    }

    model_loaded = true;
    build_category_map();

    // 目标板尺寸小(120mm正方形)，用小输入尺寸加速
    target_size = 320;  // 比640快4倍，对小目标够用

    std::cout << "[YOLO] 模型加载成功，输入尺寸=" << target_size << "\n";
    return 0;
}

int YoloDetector::detect(const cv::Mat& bgr,
                          std::vector<DetectObject>& objects,
                          float prob_thresh,
                          float nms_thresh)
{
    if (!model_loaded || bgr.empty()) return -1;

    objects.clear();

    int img_w = bgr.cols;
    int img_h = bgr.rows;
    const int max_stride = 32;

    int w = img_w;
    int h = img_h;
    float scale = 1.f;
    if (w > h) {
        scale = (float)target_size / w;
        w = target_size;
        h = (int)(h * scale);
    } else {
        scale = (float)target_size / h;
        h = target_size;
        w = (int)(w * scale);
    }

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(
        bgr.data, ncnn::Mat::PIXEL_BGR2RGB, img_w, img_h, w, h);

    int wpad = (w + max_stride - 1) / max_stride * max_stride - w;
    int hpad = (h + max_stride - 1) / max_stride * max_stride - h;
    ncnn::Mat in_pad;
    ncnn::copy_make_border(in, in_pad,
                           hpad / 2, hpad - hpad / 2,
                           wpad / 2, wpad - wpad / 2,
                           ncnn::BORDER_CONSTANT, 114.f);

    const float norm_vals[3] = {1 / 255.f, 1 / 255.f, 1 / 255.f};
    in_pad.substract_mean_normalize(0, norm_vals);

    ncnn::Extractor ex = net.create_extractor();
    ex.input("in0", in_pad);

    std::vector<DetectObject> proposals;

    // stride 8
    {
        ncnn::Mat out;
        ex.extract("out0", out);
        ncnn::Mat anchors(6);
        anchors[0] = 10.f;  anchors[1] = 13.f;
        anchors[2] = 16.f;  anchors[3] = 30.f;
        anchors[4] = 33.f;  anchors[5] = 23.f;
        std::vector<DetectObject> objs8;
        generate_proposals(anchors, 8, in_pad, out, prob_thresh, objs8);
        proposals.insert(proposals.end(), objs8.begin(), objs8.end());
    }

    // stride 16
    {
        ncnn::Mat out;
        ex.extract("out1", out);
        ncnn::Mat anchors(6);
        anchors[0] = 30.f;  anchors[1] = 61.f;
        anchors[2] = 62.f;  anchors[3] = 45.f;
        anchors[4] = 59.f;  anchors[5] = 119.f;
        std::vector<DetectObject> objs16;
        generate_proposals(anchors, 16, in_pad, out, prob_thresh, objs16);
        proposals.insert(proposals.end(), objs16.begin(), objs16.end());
    }

    // stride 32
    {
        ncnn::Mat out;
        ex.extract("out2", out);
        ncnn::Mat anchors(6);
        anchors[0] = 116.f; anchors[1] = 90.f;
        anchors[2] = 156.f; anchors[3] = 198.f;
        anchors[4] = 373.f; anchors[5] = 326.f;
        std::vector<DetectObject> objs32;
        generate_proposals(anchors, 32, in_pad, out, prob_thresh, objs32);
        proposals.insert(proposals.end(), objs32.begin(), objs32.end());
    }

    qsort_descent_inplace(proposals);

    std::vector<int> picked;
    nms_sorted_bboxes(proposals, picked, nms_thresh);

    int count = (int)picked.size();
    objects.resize(count);
    for (int i = 0; i < count; i++) {
        objects[i] = proposals[picked[i]];

        float x0 = (objects[i].rect.x - (wpad / 2)) / scale;
        float y0 = (objects[i].rect.y - (hpad / 2)) / scale;
        float x1 = (objects[i].rect.x + objects[i].rect.width - (wpad / 2)) / scale;
        float y1 = (objects[i].rect.y + objects[i].rect.height - (hpad / 2)) / scale;

        x0 = std::max(std::min(x0, (float)(img_w - 1)), 0.f);
        y0 = std::max(std::min(y0, (float)(img_h - 1)), 0.f);
        x1 = std::max(std::min(x1, (float)(img_w - 1)), 0.f);
        y1 = std::max(std::min(y1, (float)(img_h - 1)), 0.f);

        objects[i].rect.x = x0;
        objects[i].rect.y = y0;
        objects[i].rect.width  = x1 - x0;
        objects[i].rect.height = y1 - y0;
    }

    return 0;
}

TargetCategory YoloDetector::classify_target(const std::vector<DetectObject>& objects)
{
    if (objects.empty()) return TARGET_NONE;

    // 找置信度最高的检测结果
    float best_prob = 0.0f;
    int   best_label = -1;

    for (auto& obj : objects) {
        if (obj.prob > best_prob) {
            best_prob  = obj.prob;
            best_label = obj.label;
        }
    }

    if (best_label < 0 || best_label >= (int)label_to_category.size()) {
        return TARGET_NONE;
    }

    // 置信度太低则不识别（按直行处理，不罚时比错误绕行好）
    if (best_prob < 0.5f) {
        std::cout << "[YOLO] 置信度过低(" << best_prob
                  << ")，按直行处理\n";
        return TARGET_VEHICLE;  // 直行
    }

    TargetCategory cat = label_to_category[best_label];
    const char* cat_str[] = {"NONE", "WEAPON(左绕)", "SUPPLY(右绕)", "VEHICLE(直行)"};
    std::cout << "[YOLO] 检测: label=" << best_label
              << " prob=" << best_prob
              << " → " << cat_str[(int)cat] << "\n";

    return cat;
}
