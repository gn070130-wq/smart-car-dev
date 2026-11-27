#pragma once

#include <array>
#include <string>

#include <opencv2/core.hpp>

#include "vision.hpp"

struct ImageRecognitionResult
{
    int class_id;
    std::string label;
    std::array<float, 3> probabilities;
    float confidence;
    double inference_ms;
    RedAvoidanceDirection direction;
};

bool image_recognition_init(void);
bool image_recognition_infer(const cv::Mat &original_frame,
                             const VisionResult &vision_result,
                             ImageRecognitionResult &result);
void image_recognition_print(const ImageRecognitionResult &result);
