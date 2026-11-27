#include <array>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "image_recognition.hpp"
#include "seekfree_assistant.hpp"
#include "seekfree_assistant_interface.hpp"
#include "zf_common_function.hpp"
#include "zf_device_uvc.hpp"
#include "zf_driver_tcp_client.hpp"

namespace
{
constexpr uint8_t REFERENCE_CONTRAST_RATIO = 35;
constexpr uint32_t ASSISTANT_TCP_PORT = 8086;
constexpr uint8_t ASSISTANT_SEND_DIV = (UVC_FPS / 30 > 0) ? UVC_FPS / 30 : 1;

volatile std::sig_atomic_t keep_running = 1;
uint8_t refer_white_point = 0;
uint8_t white_max_refer = 0;
uint8_t white_min_refer = 0;
std::array<uint8_t, SEARCH_COL_COUNT> remote_distance{};
uint8_t assistant_image[UVC_HEIGHT][UVC_WIDTH]{};
zf_driver_tcp_client *assistant_tcp_client = nullptr;
uint8_t assistant_send_count = 0;

struct ReferenceLine
{
    uint8_t col;
    uint8_t row;
};

void sigint_handler(int)
{
    keep_running = 0;
}

uint32 assistant_transfer(const uint8 *buffer, uint32 length)
{
    if(nullptr == assistant_tcp_client || nullptr == buffer || 0 == length)
    {
        return 0;
    }
    return assistant_tcp_client->send_data(buffer, length);
}

uint32 assistant_receive(uint8 *buffer, uint32 length)
{
    if(nullptr == assistant_tcp_client || nullptr == buffer || 0 == length)
    {
        return 0;
    }
    return assistant_tcp_client->read_data(buffer, length);
}

bool initialize_assistant(zf_driver_tcp_client &client, const std::string &ip)
{
    if(ip.empty() || 0 != client.init(ip.c_str(), ASSISTANT_TCP_PORT))
    {
        return false;
    }

    assistant_tcp_client = &client;
    seekfree_assistant_interface_init(assistant_transfer, assistant_receive);
    seekfree_assistant_camera_information_config(
        SEEKFREE_ASSISTANT_MT9V03X,
        assistant_image[0],
        UVC_WIDTH,
        UVC_HEIGHT);
    seekfree_assistant_camera_boundary_config(
        NO_BOUNDARY,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    assistant_send_count = 0;
    return true;
}

void draw_line(uint8 image[UVC_HEIGHT][UVC_WIDTH],
               uint8_t x0,
               uint8_t y0,
               uint8_t x1,
               uint8_t y1,
               uint8_t color)
{
    int x = x0;
    int y = y0;
    const int end_x = x1;
    const int end_y = y1;
    const int dx = (end_x > x) ? end_x - x : x - end_x;
    const int dy = (end_y > y) ? end_y - y : y - end_y;
    const int step_x = (x < end_x) ? 1 : -1;
    const int step_y = (y < end_y) ? 1 : -1;
    int error = dx - dy;

    while(true)
    {
        image[y][x] = color;
        if(x == end_x && y == end_y)
        {
            break;
        }
        const int doubled_error = error * 2;
        if(doubled_error > -dy)
        {
            error -= dy;
            x += step_x;
        }
        if(doubled_error < dx)
        {
            error += dx;
            y += step_y;
        }
    }
}

void draw_quad(uint8 image[UVC_HEIGHT][UVC_WIDTH],
               const uint8_t corner_x[4],
               const uint8_t corner_y[4],
               uint8_t color)
{
    for(uint8_t index = 0; index < 4; ++index)
    {
        const uint8_t next = static_cast<uint8_t>((index + 1) % 4);
        draw_line(
            image,
            corner_x[index],
            corner_y[index],
            corner_x[next],
            corner_y[next],
            color);
    }
}

void send_assistant_frame(const uint8_t *gray_image,
                          const VisionResult &vision_result)
{
    ++assistant_send_count;
    if(assistant_send_count < ASSISTANT_SEND_DIV)
    {
        return;
    }
    assistant_send_count = 0;

    std::memcpy(assistant_image[0], gray_image, UVC_WIDTH * UVC_HEIGHT);
    if(vision_result.image_found)
    {
        draw_quad(
            assistant_image,
            vision_result.image_corner_x,
            vision_result.image_corner_y,
            0);
    }
    if(vision_result.red_found)
    {
        draw_quad(
            assistant_image,
            vision_result.red_corner_x,
            vision_result.red_corner_y,
            255);
    }
    seekfree_assistant_camera_send();
}

uint16_t vision_contrast_ratio(uint8_t current, uint8_t compare)
{
    const uint16_t sum = static_cast<uint16_t>(current) +
                         static_cast<uint16_t>(compare);
    int diff = static_cast<int>(current) - static_cast<int>(compare);
    if(diff < 0)
    {
        diff = -diff;
    }
    if(0 == sum)
    {
        return 0;
    }
    return static_cast<uint16_t>(diff * 200 / static_cast<int>(sum));
}

bool vision_is_white(uint8_t gray)
{
    return gray > white_max_refer;
}

bool vision_is_black(uint8_t gray)
{
    return gray < white_min_refer;
}

uint8_t calculate_bottom_reference_point(const uint8_t *image)
{
    uint16_t reference_width = REFERENCE_CENTER_WIDTH;
    if(reference_width > SEARCH_UVC_WIDTH)
    {
        reference_width = SEARCH_UVC_WIDTH;
    }

    const uint16_t start_col = (SEARCH_UVC_WIDTH - reference_width) / 2;
    const uint16_t end_col = start_col + reference_width;
    uint32_t gray_sum = 0;

    for(uint16_t row = SEARCH_UVC_HEIGHT - REFERENCE_ROW;
        row < SEARCH_UVC_HEIGHT;
        ++row)
    {
        for(uint16_t col = start_col; col < end_col; ++col)
        {
            gray_sum += image[row * SEARCH_UVC_WIDTH + col];
        }
    }

    return static_cast<uint8_t>(
        gray_sum / (REFERENCE_ROW * reference_width));
}

void update_reference_thresholds(const uint8_t *image)
{
    refer_white_point = calculate_bottom_reference_point(image);
    white_max_refer = static_cast<uint8_t>(func_limit_ab(
        static_cast<uint32_t>(refer_white_point * WHITEMAXMUL) / 10,
        BLACKPOINT,
        255));
    white_min_refer = static_cast<uint8_t>(func_limit_ab(
        static_cast<uint32_t>(refer_white_point * WHITEMINMUL) / 10,
        BLACKPOINT,
        255));
}

ReferenceLine find_longest_white_column(const uint8_t *image)
{
    remote_distance.fill(SEARCH_UVC_HEIGHT - 1);

    for(size_t col_index = 0; col_index < remote_distance.size(); ++col_index)
    {
        const int col = func_limit_ab(
            static_cast<int>(col_index) * CONTRASTOFFSET,
            0,
            SEARCH_UVC_WIDTH - 1);

        for(int row = SEARCH_UVC_HEIGHT - 1;
            row > SEARCHSTOPROW;
            row -= CONTRASTOFFSET)
        {
            const int compare_row = func_limit_ab(
                row - CONTRASTOFFSET,
                SEARCHSTOPROW,
                SEARCH_UVC_HEIGHT - 1);
            const uint8_t current_gray =
                image[row * SEARCH_UVC_WIDTH + col];
            const uint8_t compare_gray =
                image[compare_row * SEARCH_UVC_WIDTH + col];

            if(vision_is_white(current_gray))
            {
                continue;
            }
            if(vision_is_black(current_gray) ||
               (current_gray <= compare_gray &&
                vision_contrast_ratio(current_gray, compare_gray) >
                    REFERENCE_CONTRAST_RATIO))
            {
                remote_distance[col_index] = static_cast<uint8_t>(row);
                break;
            }
        }
    }

    size_t best_index = 0;
    for(size_t index = 1; index < remote_distance.size(); ++index)
    {
        if(remote_distance[index] < remote_distance[best_index])
        {
            best_index = index;
        }
    }

    ReferenceLine reference{};
    reference.col = static_cast<uint8_t>(func_limit_ab(
        static_cast<int>(best_index) * CONTRASTOFFSET,
        0,
        SEARCH_UVC_WIDTH - 1));
    reference.row = remote_distance[best_index];
    return reference;
}

void reset_detection_result(VisionResult &result)
{
    result.red_found = 0;
    result.red_x = 0;
    result.red_y = 0;
    result.red_w = 0;
    result.red_h = 0;
    result.image_found = 0;
    result.image_x = 0;
    result.image_y = 0;
    result.image_w = 0;
    result.image_h = 0;
    for(uint8_t index = 0; index < 4; ++index)
    {
        result.red_corner_x[index] = 0;
        result.red_corner_y[index] = 0;
        result.image_corner_x[index] = 0;
        result.image_corner_y[index] = 0;
    }
}

// Keep this detector in sync with project/user/vision.cpp::Detect_Red_Marker.
void detect_red_marker(const cv::Mat &color_frame, VisionResult &result)
{
    reset_detection_result(result);
    if(color_frame.empty())
    {
        return;
    }

    const float result_scale_x = static_cast<float>(SEARCH_UVC_WIDTH) /
                                 static_cast<float>(color_frame.cols);
    const float result_scale_y = static_cast<float>(SEARCH_UVC_HEIGHT) /
                                 static_cast<float>(color_frame.rows);
    const int detect_top_row =
        (result.reference_row > RED_MARKER_DETECT_TOP_ROW) ?
        result.reference_row : RED_MARKER_DETECT_TOP_ROW;

    cv::Mat hsv;
    cv::Mat mask_low;
    cv::Mat mask_high;
    cv::Mat red_mask;
    cv::cvtColor(color_frame, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(
        hsv,
        cv::Scalar(RED_HUE_LOW_MIN, RED_SATURATION_MIN, RED_BRIGHTNESS_MIN),
        cv::Scalar(RED_HUE_LOW_MAX, RED_HSV_CHANNEL_MAX, RED_HSV_CHANNEL_MAX),
        mask_low);
    cv::inRange(
        hsv,
        cv::Scalar(RED_HUE_HIGH_MIN, RED_SATURATION_MIN, RED_BRIGHTNESS_MIN),
        cv::Scalar(RED_HUE_HIGH_MAX, RED_HSV_CHANNEL_MAX, RED_HSV_CHANNEL_MAX),
        mask_high);
    cv::bitwise_or(mask_low, mask_high, red_mask);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(
        red_mask,
        contours,
        cv::RETR_EXTERNAL,
        cv::CHAIN_APPROX_SIMPLE);

    int best_index = -1;
    int best_bottom = -1;
    double best_area = 0.0;

    for(size_t index = 0; index < contours.size(); ++index)
    {
        const cv::Rect box = cv::boundingRect(contours[index]);
        const cv::RotatedRect rotated_box = cv::minAreaRect(contours[index]);
        const double area = static_cast<double>(cv::countNonZero(red_mask(box)));
        const float rotated_side_a = rotated_box.size.width + 1.0f;
        const float rotated_side_b = rotated_box.size.height + 1.0f;
        const float long_side = (rotated_side_a >= rotated_side_b) ?
                                rotated_side_a : rotated_side_b;
        const float short_side = (rotated_side_a < rotated_side_b) ?
                                 rotated_side_a : rotated_side_b;
        const float aspect = (short_side <= 0.0f) ?
                             0.0f : long_side / short_side;
        const int bottom = box.y + box.height;
        const int result_left = cvRound(box.x * result_scale_x);
        const int result_right = cvRound((box.x + box.width) * result_scale_x);
        const int result_top = cvRound(box.y * result_scale_y);
        const int result_bottom = cvRound(bottom * result_scale_y);

        if(result_left < RED_MARKER_DETECT_LEFT_MARGIN ||
           result_right > SEARCH_UVC_WIDTH - RED_MARKER_DETECT_RIGHT_MARGIN ||
           result_top < detect_top_row ||
           result_bottom > SEARCH_UVC_HEIGHT - RED_MARKER_DETECT_BOTTOM_MARGIN)
        {
            continue;
        }
        if(long_side < RED_MARKER_MIN_LONG_SIDE ||
           short_side < RED_MARKER_MIN_SHORT_SIDE ||
           area < RED_MARKER_MIN_AREA ||
           area > RED_MARKER_MAX_AREA)
        {
            continue;
        }
        if(aspect < RED_MARKER_MIN_ASPECT || aspect > RED_MARKER_MAX_ASPECT)
        {
            continue;
        }
        if(bottom > best_bottom || (bottom == best_bottom && area > best_area))
        {
            best_index = static_cast<int>(index);
            best_bottom = bottom;
            best_area = area;
        }
    }

    if(best_index < 0)
    {
        return;
    }

    const cv::Rect red_box = cv::boundingRect(contours[static_cast<size_t>(best_index)]);
    const int red_x = func_limit_ab(
        cvRound(red_box.x * result_scale_x),
        0,
        SEARCH_UVC_WIDTH - 1);
    const int red_y = func_limit_ab(
        cvRound(red_box.y * result_scale_y),
        0,
        SEARCH_UVC_HEIGHT - 1);
    const int red_right = func_limit_ab(
        cvRound((red_box.x + red_box.width) * result_scale_x),
        red_x + 1,
        SEARCH_UVC_WIDTH);
    const int red_bottom = func_limit_ab(
        cvRound((red_box.y + red_box.height) * result_scale_y),
        red_y + 1,
        SEARCH_UVC_HEIGHT);
    const int red_w = red_right - red_x;
    const int red_h = red_bottom - red_y;

    result.red_found = 1;
    result.red_x = static_cast<uint8_t>(red_x);
    result.red_y = static_cast<uint8_t>(red_y);
    result.red_w = static_cast<uint8_t>(red_w);
    result.red_h = static_cast<uint8_t>(red_h);

    result.red_corner_x[0] = static_cast<uint8_t>(red_x);
    result.red_corner_y[0] = static_cast<uint8_t>(red_y);
    result.red_corner_x[1] = static_cast<uint8_t>(red_right - 1);
    result.red_corner_y[1] = static_cast<uint8_t>(red_y);
    result.red_corner_x[2] = static_cast<uint8_t>(red_right - 1);
    result.red_corner_y[2] = static_cast<uint8_t>(red_bottom - 1);
    result.red_corner_x[3] = static_cast<uint8_t>(red_x);
    result.red_corner_y[3] = static_cast<uint8_t>(red_bottom - 1);

    const int red_center_x = red_x + red_w / 2;
    const int red_bottom_y = red_y + red_h;
    const int image_x = func_limit_ab(
        red_center_x - IMAGE_ROI_SIZE / 2,
        0,
        SEARCH_UVC_WIDTH - IMAGE_ROI_SIZE);
    const int image_y = func_limit_ab(
        red_bottom_y - IMAGE_ROI_SIZE,
        0,
        SEARCH_UVC_HEIGHT - IMAGE_ROI_SIZE);
    const int image_right = image_x + IMAGE_ROI_SIZE - 1;
    const int image_bottom = image_y + IMAGE_ROI_SIZE - 1;

    result.image_found = 1;
    result.image_x = static_cast<uint8_t>(image_x);
    result.image_y = static_cast<uint8_t>(image_y);
    result.image_w = IMAGE_ROI_SIZE;
    result.image_h = IMAGE_ROI_SIZE;

    result.image_corner_x[0] = static_cast<uint8_t>(image_x);
    result.image_corner_y[0] = static_cast<uint8_t>(image_y);
    result.image_corner_x[1] = static_cast<uint8_t>(image_right);
    result.image_corner_y[1] = static_cast<uint8_t>(image_y);
    result.image_corner_x[2] = static_cast<uint8_t>(image_right);
    result.image_corner_y[2] = static_cast<uint8_t>(image_bottom);
    result.image_corner_x[3] = static_cast<uint8_t>(image_x);
    result.image_corner_y[3] = static_cast<uint8_t>(image_bottom);
}

bool parse_recognition_bottom_row(const std::string &text, int &row)
{
    char *end = nullptr;
    const unsigned long value = std::strtoul(text.c_str(), &end, 10);
    if(text.empty() || end == text.c_str() || *end != '\0' ||
       value > SEARCH_UVC_HEIGHT)
    {
        return false;
    }
    row = static_cast<int>(value);
    return true;
}

} // namespace

int main()
{
    std::signal(SIGINT, sigint_handler);

    std::string assistant_ip;
    std::printf("请输入逐飞助手 IP: ");
    std::fflush(stdout);
    if(!std::getline(std::cin, assistant_ip) || assistant_ip.empty())
    {
        std::fprintf(stderr, "MODEL_TEST: assistant IP is empty\n");
        return 1;
    }

    std::string recognition_bottom_row_text;
    std::printf("请输入红块底边最小行号 (0-%d，底边行号 > 此值): ",
                SEARCH_UVC_HEIGHT);
    std::fflush(stdout);
    int recognition_bottom_row = 0;
    if(!std::getline(std::cin, recognition_bottom_row_text) ||
       !parse_recognition_bottom_row(recognition_bottom_row_text,
                                     recognition_bottom_row))
    {
        std::fprintf(stderr,
                     "MODEL_TEST: invalid recognition bottom row "
                     "(expected 0-%d)\n",
                     SEARCH_UVC_HEIGHT);
        return 1;
    }

    zf_driver_tcp_client tcp_client;
    if(!initialize_assistant(tcp_client, assistant_ip))
    {
        std::fprintf(
            stderr,
            "MODEL_TEST: failed to connect Seekfree Assistant at %s:%u\n",
            assistant_ip.c_str(),
            ASSISTANT_TCP_PORT);
        return 1;
    }
    std::printf(
        "MODEL_TEST: Seekfree Assistant connected at %s:%u\n",
        assistant_ip.c_str(),
        ASSISTANT_TCP_PORT);

    zf_device_uvc uvc_dev;
    if(0 != uvc_dev.init(UVC_PATH))
    {
        std::fprintf(stderr, "MODEL_TEST: UVC initialization failed\n");
        return 1;
    }
    if(!image_recognition_init())
    {
        std::fprintf(stderr, "MODEL_TEST: model initialization failed\n");
        return 1;
    }

    std::printf(
        "MODEL_TEST: continuous inference starts when red bottom row > %d; "
        "Ctrl+C to stop\n",
        recognition_bottom_row);

    VisionResult vision_result{};
    uint64_t inference_count = 0;

    while(keep_running)
    {
        if(uvc_dev.wait_image_refresh() < 0)
        {
            std::fprintf(stderr, "MODEL_TEST: camera frame acquisition failed\n");
            return 1;
        }

        const uint8_t *gray_image = uvc_dev.get_gray_image_ptr();
        if(nullptr == gray_image)
        {
            continue;
        }

        update_reference_thresholds(gray_image);
        const ReferenceLine reference = find_longest_white_column(gray_image);
        vision_result.reference_col = reference.col;
        vision_result.reference_row = reference.row;
        detect_red_marker(uvc_dev.get_frame_resized(), vision_result);
        send_assistant_frame(gray_image, vision_result);

        if(0 == vision_result.red_found)
        {
            continue;
        }

        const int bottom_row = vision_result.red_y + vision_result.red_h;
        if(bottom_row <= recognition_bottom_row)
        {
            continue;
        }

        ImageRecognitionResult recognition_result;
        if(!image_recognition_infer(
               uvc_dev.get_frame_mjpg(),
               vision_result,
               recognition_result))
        {
            continue;
        }

        ++inference_count;
        std::printf(
            "[MODEL_TEST #%llu] reference=(%u,%u), red=(%u,%u,%u,%u), "
            "roi=(%u,%u,%u,%u), bottom_row=%d\n",
            static_cast<unsigned long long>(inference_count),
            vision_result.reference_col,
            vision_result.reference_row,
            vision_result.red_x,
            vision_result.red_y,
            vision_result.red_w,
            vision_result.red_h,
            vision_result.image_x,
            vision_result.image_y,
            vision_result.image_w,
            vision_result.image_h,
            bottom_row);
        image_recognition_print(recognition_result);
    }

    std::printf("MODEL_TEST: stopped\n");
    return 0;
}
