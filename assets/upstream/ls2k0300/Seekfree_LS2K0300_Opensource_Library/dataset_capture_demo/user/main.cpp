/*********************************************************************************************************************
* LS2K0300 Opensourec Library is based on the official SDK interfaces.
* Copyright (c) 2022 SEEKFREE
*
* This file is part of the LS2K0300 open-source library and is distributed under GPL-3.0-or-later.
********************************************************************************************************************/

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <fcntl.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "zf_common_function.hpp"
#include "zf_device_imu.hpp"
#include "zf_device_uvc.hpp"
#include "zf_driver_delay.hpp"
#include "zf_driver_encoder.hpp"
#include "zf_driver_gpio.hpp"
#include "zf_driver_pit.hpp"
#include "zf_driver_pwm.hpp"
#include "zf_driver_tcp_client.hpp"

namespace
{
constexpr uint32_t DEFAULT_SERVER_PORT = 8087;
constexpr uint32_t CAPTURE_INTERVAL_MS = 125;
constexpr uint32_t CONTROL_INTERVAL_MS = 5;
constexpr uint32_t DIRECTION_CHANGE_PAUSE_MS = 250;
constexpr uint32_t DIRECTION_CHANGE_PAUSE_TICKS =
    (DIRECTION_CHANGE_PAUSE_MS + CONTROL_INTERVAL_MS - 1) / CONTROL_INTERVAL_MS;

constexpr float GYRO_LSB_PER_DPS = 16.4f;
constexpr float YAW_LIMIT_DEGREES = 20.0f;
constexpr float TARGET_YAW_RATE_DPS = 10.0f;
constexpr float GYRO_KP = 0.13f;
constexpr float GYRO_KD = 0.0f;
constexpr float CENTER_YAW_KP = 0.8f;
constexpr float MOVE_HEADING_KP = 0.8f;
constexpr float CENTER_YAW_TOLERANCE_DEGREES = 1.0f;
constexpr float MOVE_TARGET_SPEED = 50.0f;
constexpr int32_t MOVE_SEGMENT_ENCODER_COUNT = 150;
constexpr int32_t MOVE_ROUTE_LIMIT = 4500;

constexpr float SPEED_LEFT_KP = 0.35f;
constexpr float SPEED_LEFT_KI = 0.01f;
constexpr float SPEED_LEFT_KD = 0.0f;
constexpr float SPEED_RIGHT_KP = 0.35f;
constexpr float SPEED_RIGHT_KI = 0.012f;
constexpr float SPEED_RIGHT_KD = 0.0f;
constexpr float MAX_PWM_DUTY_PERCENT = 30.0f;

constexpr int MOTOR_LEFT_DIR_LEVEL = 1;
constexpr int MOTOR_RIGHT_DIR_LEVEL = 1;
constexpr int ENCODER_LEFT_SIGN = 1;
constexpr int ENCODER_RIGHT_SIGN = -1;

constexpr int SEARCH_UVC_WIDTH = UVC_WIDTH;
constexpr int SEARCH_UVC_HEIGHT = UVC_HEIGHT;
constexpr int IMAGE_ROI_SIZE = 32;
constexpr int MODEL_INPUT_WIDTH = 64;
constexpr int MODEL_INPUT_HEIGHT = 64;

// These values and the detector below intentionally match project/user/vision.hpp and vision.cpp.
constexpr int RED_HUE_LOW_MIN = 0;
constexpr int RED_HUE_LOW_MAX = 10;
constexpr int RED_HUE_HIGH_MIN = 170;
constexpr int RED_HUE_HIGH_MAX = 180;
constexpr int RED_SATURATION_MIN = 90;
constexpr int RED_BRIGHTNESS_MIN = 80;
constexpr int RED_HSV_CHANNEL_MAX = 255;
constexpr float RED_MARKER_MIN_LONG_SIDE = 5.0f;
constexpr float RED_MARKER_MIN_SHORT_SIDE = 1.0f;
constexpr double RED_MARKER_MIN_AREA = 5.0;
constexpr float RED_MARKER_MIN_ASPECT = 1.5f;
constexpr float RED_MARKER_MAX_ASPECT = 15.0f;
constexpr float RED_MARKER_MIN_FILL_RATIO = 0.45f;

constexpr size_t FRAME_HEADER_SIZE = 20;
constexpr char FRAME_MAGIC[4] = {'D', 'S', '0', '1'};

struct CaptureRoi
{
    uint8_t red_found = 0;
    uint8_t red_x = 0;
    uint8_t red_y = 0;
    uint8_t red_w = 0;
    uint8_t red_h = 0;
    uint8_t red_corner_x[4] = {};
    uint8_t red_corner_y[4] = {};
    uint8_t image_found = 0;
    uint8_t image_x = 0;
    uint8_t image_y = 0;
    uint8_t image_w = 0;
    uint8_t image_h = 0;
    uint8_t image_corner_x[4] = {};
    uint8_t image_corner_y[4] = {};
};

struct SpeedController
{
    float error_0 = 0.0f;
    float error_1 = 0.0f;
    float error_2 = 0.0f;
    float output = 0.0f;
    float kp;
    float ki;
    float kd;
};

enum class MotionState : uint8_t
{
    ROTATE_TO_POSITIVE,
    PAUSE_AFTER_POSITIVE,
    ROTATE_TO_NEGATIVE,
    PAUSE_AFTER_NEGATIVE,
    ROTATE_TO_CENTER,
    MOVE_FORWARD,
    MOVE_BACKWARD,
};

zf_driver_pit pit_timer;
zf_device_uvc uvc_dev;
zf_device_imu imu_dev;
zf_driver_tcp_client tcp_client;

zf_driver_encoder encoder_left(ZF_ENCODER_QUAD_1);
zf_driver_encoder encoder_right(ZF_ENCODER_QUAD_2);
zf_driver_pwm motor_pwm_left(ZF_PWM_MOTOR_1);
zf_driver_pwm motor_pwm_right(ZF_PWM_MOTOR_2);
zf_driver_gpio motor_dir_left(ZF_GPIO_MOTOR_1, O_RDWR);
zf_driver_gpio motor_dir_right(ZF_GPIO_MOTOR_2, O_RDWR);

pwm_info motor_pwm_left_info;
pwm_info motor_pwm_right_info;
SpeedController speed_left{0.0f, 0.0f, 0.0f, 0.0f,
                           SPEED_LEFT_KP, SPEED_LEFT_KI, SPEED_LEFT_KD};
SpeedController speed_right{0.0f, 0.0f, 0.0f, 0.0f,
                            SPEED_RIGHT_KP, SPEED_RIGHT_KI, SPEED_RIGHT_KD};

float gyro_z_offset = 0.0f;
float gyro_error_0 = 0.0f;
float gyro_error_1 = 0.0f;
MotionState motion_state = MotionState::ROTATE_TO_POSITIVE;
uint32_t motion_state_ticks = 0;
int32_t route_encoder_sum = 0;
int move_direction = 1;
int32_t move_target_encoder_sum = MOVE_SEGMENT_ENCODER_COUNT * 2;
int capture_bottom_row_min = 0;
int capture_bottom_row_max = SEARCH_UVC_HEIGHT;
std::atomic<float> yaw_degrees{0.0f};
std::atomic<int> motion_state_report{static_cast<int>(MotionState::ROTATE_TO_POSITIVE)};
std::atomic<int32_t> route_position_report{0};
std::atomic<int> red_bottom_row_report{-1};

void stop_motors()
{
    motor_pwm_left.set_duty(0);
    motor_pwm_right.set_duty(0);
    motor_dir_left.set_level(0);
    motor_dir_right.set_level(0);
}

void cleanup()
{
    pit_timer.stop();
    stop_motors();
    std::printf("dataset capture stopped\n");
}

void sigint_handler(int)
{
    std::exit(0);
}

void set_motor_output(zf_driver_gpio &direction,
                      zf_driver_pwm &pwm,
                      const pwm_info &info,
                      float output,
                      int forward_level)
{
    output = func_limit_ab(output, -MAX_PWM_DUTY_PERCENT, MAX_PWM_DUTY_PERCENT);
    if(output >= 0.0f)
    {
        direction.set_level(forward_level);
    }
    else
    {
        direction.set_level(!forward_level);
        output = -output;
    }

    pwm.set_duty(static_cast<uint16>(output * (info.duty_max / 100)));
}

float update_speed_controller(SpeedController &controller, float target, float actual)
{
    controller.error_2 = controller.error_1;
    controller.error_1 = controller.error_0;
    controller.error_0 = target - actual;
    controller.output += controller.kp * (controller.error_0 - controller.error_1)
                       + controller.ki * controller.error_0
                       + controller.kd * (controller.error_0
                                          - 2.0f * controller.error_1
                                          + controller.error_2);
    controller.output = func_limit_ab(controller.output,
                                      -MAX_PWM_DUTY_PERCENT,
                                      MAX_PWM_DUTY_PERCENT);
    return controller.output;
}

void reset_speed_controller(SpeedController &controller)
{
    controller.error_0 = 0.0f;
    controller.error_1 = 0.0f;
    controller.error_2 = 0.0f;
    controller.output = 0.0f;
}

void change_motion_state(MotionState next_state)
{
    if(motion_state == next_state)
    {
        return;
    }

    motion_state = next_state;
    motion_state_ticks = 0;
    gyro_error_0 = 0.0f;
    gyro_error_1 = 0.0f;
    reset_speed_controller(speed_left);
    reset_speed_controller(speed_right);
}

void start_next_move()
{
    const int bottom_row = red_bottom_row_report.load(
        std::memory_order_relaxed);
    if(bottom_row >= 0)
    {
        if(bottom_row >= capture_bottom_row_max)
        {
            move_direction = -1;
        }
        else if(bottom_row <= capture_bottom_row_min)
        {
            move_direction = 1;
        }
    }

    const int32_t route_position = route_encoder_sum / 2;
    if(route_position >= MOVE_ROUTE_LIMIT)
    {
        move_direction = -1;
    }
    else if(route_position <= -MOVE_ROUTE_LIMIT)
    {
        move_direction = 1;
    }

    move_target_encoder_sum = func_limit_ab(
        route_encoder_sum + move_direction * MOVE_SEGMENT_ENCODER_COUNT * 2,
        -MOVE_ROUTE_LIMIT * 2,
        MOVE_ROUTE_LIMIT * 2);
    change_motion_state((move_direction > 0) ? MotionState::MOVE_FORWARD
                                             : MotionState::MOVE_BACKWARD);
}

float update_gyro_controller(float target_yaw_rate_dps, float actual_gyro_z)
{
    const float target_gyro_z = target_yaw_rate_dps * GYRO_LSB_PER_DPS;
    gyro_error_1 = gyro_error_0;
    gyro_error_0 = target_gyro_z - actual_gyro_z;
    return GYRO_KP * gyro_error_0
         + GYRO_KD * (gyro_error_0 - gyro_error_1);
}

const char *motion_state_name(int state)
{
    switch(static_cast<MotionState>(state))
    {
        case MotionState::ROTATE_TO_POSITIVE: return "rotate_to_+20";
        case MotionState::PAUSE_AFTER_POSITIVE: return "pause_at_+20";
        case MotionState::ROTATE_TO_NEGATIVE: return "rotate_to_-20";
        case MotionState::PAUSE_AFTER_NEGATIVE: return "pause_at_-20";
        case MotionState::ROTATE_TO_CENTER:   return "rotate_to_center";
        case MotionState::MOVE_FORWARD:       return "move_forward";
        case MotionState::MOVE_BACKWARD:      return "move_backward";
        default:                              return "unknown";
    }
}

void control_tick()
{
    const float actual_gyro_z = -(static_cast<float>(imu_dev.get_gyro_z()) - gyro_z_offset);
    float angle = yaw_degrees.load(std::memory_order_relaxed);
    angle += actual_gyro_z / GYRO_LSB_PER_DPS * (CONTROL_INTERVAL_MS / 1000.0f);
    yaw_degrees.store(angle, std::memory_order_relaxed);

    const int32_t actual_speed_left = encoder_left.get_count() * ENCODER_LEFT_SIGN;
    const int32_t actual_speed_right = encoder_right.get_count() * ENCODER_RIGHT_SIGN;
    encoder_left.clear_count();
    encoder_right.clear_count();

    if(MotionState::MOVE_FORWARD == motion_state ||
       MotionState::MOVE_BACKWARD == motion_state)
    {
        route_encoder_sum += actual_speed_left + actual_speed_right;
    }

    switch(motion_state)
    {
        case MotionState::ROTATE_TO_POSITIVE:
            if(angle >= YAW_LIMIT_DEGREES)
            {
                change_motion_state(MotionState::PAUSE_AFTER_POSITIVE);
            }
            break;

        case MotionState::PAUSE_AFTER_POSITIVE:
            motion_state_ticks++;
            if(motion_state_ticks >= DIRECTION_CHANGE_PAUSE_TICKS)
            {
                change_motion_state(MotionState::ROTATE_TO_NEGATIVE);
            }
            break;

        case MotionState::ROTATE_TO_NEGATIVE:
            if(angle <= -YAW_LIMIT_DEGREES)
            {
                change_motion_state(MotionState::PAUSE_AFTER_NEGATIVE);
            }
            break;

        case MotionState::PAUSE_AFTER_NEGATIVE:
            motion_state_ticks++;
            if(motion_state_ticks >= DIRECTION_CHANGE_PAUSE_TICKS)
            {
                change_motion_state(MotionState::ROTATE_TO_CENTER);
            }
            break;

        case MotionState::ROTATE_TO_CENTER:
            if(std::fabs(angle) <= CENTER_YAW_TOLERANCE_DEGREES)
            {
                angle = 0.0f;
                yaw_degrees.store(angle, std::memory_order_relaxed);
                start_next_move();
            }
            break;

        case MotionState::MOVE_FORWARD:
            if(route_encoder_sum >= move_target_encoder_sum)
            {
                change_motion_state(MotionState::ROTATE_TO_POSITIVE);
            }
            break;

        case MotionState::MOVE_BACKWARD:
            if(route_encoder_sum <= move_target_encoder_sum)
            {
                change_motion_state(MotionState::ROTATE_TO_POSITIVE);
            }
            break;
    }

    if(MotionState::PAUSE_AFTER_POSITIVE == motion_state ||
       MotionState::PAUSE_AFTER_NEGATIVE == motion_state)
    {
        stop_motors();
        motion_state_report.store(static_cast<int>(motion_state),
                                  std::memory_order_relaxed);
        route_position_report.store(route_encoder_sum / 2,
                                    std::memory_order_relaxed);
        return;
    }

    float base_speed = 0.0f;
    float target_yaw_rate_dps = 0.0f;
    switch(motion_state)
    {
        case MotionState::ROTATE_TO_POSITIVE:
            target_yaw_rate_dps = TARGET_YAW_RATE_DPS;
            break;

        case MotionState::PAUSE_AFTER_POSITIVE:
        case MotionState::PAUSE_AFTER_NEGATIVE:
            break;

        case MotionState::ROTATE_TO_NEGATIVE:
            target_yaw_rate_dps = -TARGET_YAW_RATE_DPS;
            break;

        case MotionState::ROTATE_TO_CENTER:
            target_yaw_rate_dps = func_limit(-angle * CENTER_YAW_KP,
                                              TARGET_YAW_RATE_DPS);
            break;

        case MotionState::MOVE_FORWARD:
            base_speed = MOVE_TARGET_SPEED;
            target_yaw_rate_dps = func_limit(-angle * MOVE_HEADING_KP,
                                              TARGET_YAW_RATE_DPS);
            break;

        case MotionState::MOVE_BACKWARD:
            base_speed = -MOVE_TARGET_SPEED;
            target_yaw_rate_dps = func_limit(-angle * MOVE_HEADING_KP,
                                              TARGET_YAW_RATE_DPS);
            break;
    }

    const float gyro_correction_speed = update_gyro_controller(target_yaw_rate_dps,
                                                                actual_gyro_z);
    const float target_speed_left = base_speed + gyro_correction_speed;
    const float target_speed_right = base_speed - gyro_correction_speed;

    const float output_left = update_speed_controller(speed_left,
                                                       target_speed_left,
                                                       actual_speed_left);
    const float output_right = update_speed_controller(speed_right,
                                                        target_speed_right,
                                                        actual_speed_right);
    set_motor_output(motor_dir_left,
                     motor_pwm_left,
                     motor_pwm_left_info,
                     output_left,
                     MOTOR_LEFT_DIR_LEVEL);
    set_motor_output(motor_dir_right,
                     motor_pwm_right,
                     motor_pwm_right_info,
                     output_right,
                     MOTOR_RIGHT_DIR_LEVEL);

    motion_state_report.store(static_cast<int>(motion_state), std::memory_order_relaxed);
    route_position_report.store(route_encoder_sum / 2, std::memory_order_relaxed);
}

void yaw_offset_calibration()
{
    const uint16 sample_count = 100;
    int32 sum = 0;
    for(uint16 i = 0; i < sample_count; i++)
    {
        sum += imu_dev.get_gyro_z();
        system_delay_ms(5);
    }
    gyro_z_offset = static_cast<float>(sum) / sample_count;
}

void detect_red_marker(const cv::Mat &color_frame, CaptureRoi &result)
{
    result = CaptureRoi{};
    if(color_frame.empty())
    {
        return;
    }

    const cv::Mat &frame = color_frame;
    const float result_scale_x = static_cast<float>(SEARCH_UVC_WIDTH) /
                                 static_cast<float>(frame.cols);
    const float result_scale_y = static_cast<float>(SEARCH_UVC_HEIGHT) /
                                 static_cast<float>(frame.rows);

    cv::Mat hsv, mask_low, mask_high, red_mask;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv,
                cv::Scalar(RED_HUE_LOW_MIN, RED_SATURATION_MIN, RED_BRIGHTNESS_MIN),
                cv::Scalar(RED_HUE_LOW_MAX, RED_HSV_CHANNEL_MAX, RED_HSV_CHANNEL_MAX),
                mask_low);
    cv::inRange(hsv,
                cv::Scalar(RED_HUE_HIGH_MIN, RED_SATURATION_MIN, RED_BRIGHTNESS_MIN),
                cv::Scalar(RED_HUE_HIGH_MAX, RED_HSV_CHANNEL_MAX, RED_HSV_CHANNEL_MAX),
                mask_high);
    cv::bitwise_or(mask_low, mask_high, red_mask);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(red_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    int best_index = -1;
    int best_bottom = -1;
    double best_area = 0.0;
    cv::RotatedRect best_rotated_box;
    for(size_t index = 0; index < contours.size(); index++)
    {
        cv::Rect box = cv::boundingRect(contours[index]);
        cv::RotatedRect rotated_box = cv::minAreaRect(contours[index]);
        const double area = static_cast<double>(cv::countNonZero(red_mask(box)));
        float long_side = static_cast<float>(box.width);
        float short_side = static_cast<float>(box.height);
        if(long_side < short_side)
        {
            const float temp = long_side;
            long_side = short_side;
            short_side = temp;
        }

        const float box_area = static_cast<float>(box.width * box.height);
        const float aspect = (short_side <= 0.0f) ? 0.0f : (long_side / short_side);
        const float fill_ratio = (box_area <= 0.0f) ? 0.0f
                                                    : (static_cast<float>(area) / box_area);
        const int bottom = box.y + box.height;

        if(long_side < RED_MARKER_MIN_LONG_SIDE ||
           short_side < RED_MARKER_MIN_SHORT_SIDE ||
           area < RED_MARKER_MIN_AREA)
        {
            continue;
        }
        if(aspect < RED_MARKER_MIN_ASPECT || aspect > RED_MARKER_MAX_ASPECT)
        {
            continue;
        }
        if(fill_ratio < RED_MARKER_MIN_FILL_RATIO)
        {
            continue;
        }
        if(bottom > best_bottom || (bottom == best_bottom && area > best_area))
        {
            best_index = static_cast<int>(index);
            best_bottom = bottom;
            best_area = area;
            best_rotated_box = rotated_box;
        }
    }

    if(best_index < 0)
    {
        return;
    }

    const cv::Rect red_box = cv::boundingRect(contours[static_cast<size_t>(best_index)]);
    const int red_x = func_limit_ab(cvRound(red_box.x * result_scale_x),
                                    0,
                                    SEARCH_UVC_WIDTH - 1);
    const int red_y = func_limit_ab(cvRound(red_box.y * result_scale_y),
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

    cv::Point2f red_points[4];
    best_rotated_box.points(red_points);
    for(uint8_t index = 0; index < 4; index++)
    {
        result.red_corner_x[index] = static_cast<uint8_t>(func_limit_ab(
            cvRound(red_points[index].x * result_scale_x),
            0,
            SEARCH_UVC_WIDTH - 1));
        result.red_corner_y[index] = static_cast<uint8_t>(func_limit_ab(
            cvRound(red_points[index].y * result_scale_y),
            0,
            SEARCH_UVC_HEIGHT - 1));
    }

    const int red_center_x = red_x + red_w / 2;
    const int red_bottom_y = red_y + red_h;
    const int image_x = func_limit_ab(red_center_x - IMAGE_ROI_SIZE / 2,
                                      0,
                                      SEARCH_UVC_WIDTH - IMAGE_ROI_SIZE);
    const int image_y = func_limit_ab(red_bottom_y - IMAGE_ROI_SIZE,
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

bool build_model_input(const cv::Mat &original_frame,
                       const CaptureRoi &vision_result,
                       cv::Mat &model_input)
{
    if(original_frame.empty() || 0 == vision_result.image_found)
    {
        return false;
    }

    const float scale_x = static_cast<float>(original_frame.cols) /
                          static_cast<float>(SEARCH_UVC_WIDTH);
    const float scale_y = static_cast<float>(original_frame.rows) /
                          static_cast<float>(SEARCH_UVC_HEIGHT);
    if(0 == vision_result.image_w || 0 == vision_result.image_h)
    {
        return false;
    }

    const int roi_left = cvRound(vision_result.image_x * scale_x);
    const int roi_top = cvRound(vision_result.image_y * scale_y);
    const int roi_right = cvRound((vision_result.image_x + vision_result.image_w) * scale_x);
    const int roi_bottom = cvRound((vision_result.image_y + vision_result.image_h) * scale_y);
    const cv::Rect roi(roi_left,
                       roi_top,
                       roi_right - roi_left,
                       roi_bottom - roi_top);
    const cv::Rect frame_bounds(0, 0, original_frame.cols, original_frame.rows);
    if(roi.width <= 0 || roi.height <= 0 || (roi & frame_bounds) != roi)
    {
        return false;
    }

    model_input = original_frame(roi).clone();
    return model_input.cols == MODEL_INPUT_WIDTH &&
           model_input.rows == MODEL_INPUT_HEIGHT;
}

void write_u16_be(uint8_t *destination, uint16_t value)
{
    destination[0] = static_cast<uint8_t>((value >> 8) & 0xff);
    destination[1] = static_cast<uint8_t>(value & 0xff);
}

void write_u32_be(uint8_t *destination, uint32_t value)
{
    destination[0] = static_cast<uint8_t>((value >> 24) & 0xff);
    destination[1] = static_cast<uint8_t>((value >> 16) & 0xff);
    destination[2] = static_cast<uint8_t>((value >> 8) & 0xff);
    destination[3] = static_cast<uint8_t>(value & 0xff);
}

bool send_model_input(const cv::Mat &model_input, uint32_t sequence)
{
    std::vector<uint8_t> png_data;
    if(!cv::imencode(".png", model_input, png_data))
    {
        std::fprintf(stderr, "PNG encode failed\n");
        return true;
    }

    uint8_t header[FRAME_HEADER_SIZE] = {};
    std::memcpy(header, FRAME_MAGIC, sizeof(FRAME_MAGIC));
    write_u32_be(header + 4, static_cast<uint32_t>(png_data.size()));
    write_u32_be(header + 8, sequence);
    const int32_t yaw_millidegrees = static_cast<int32_t>(
        yaw_degrees.load(std::memory_order_relaxed) * 1000.0f);
    write_u32_be(header + 12, static_cast<uint32_t>(yaw_millidegrees));
    write_u16_be(header + 16, static_cast<uint16_t>(model_input.cols));
    write_u16_be(header + 18, static_cast<uint16_t>(model_input.rows));

    if(tcp_client.send_data(header, sizeof(header)) != sizeof(header))
    {
        return false;
    }
    if(tcp_client.send_data(png_data.data(), static_cast<uint32_t>(png_data.size())) !=
       png_data.size())
    {
        return false;
    }

    std::printf("sent image %u, yaw=%.2f deg, bytes=%zu\n",
                sequence,
                yaw_millidegrees / 1000.0f,
                png_data.size());
    return true;
}

bool stop_requested()
{
    uint8 command = 0;
    const uint32 read_length = tcp_client.read_data(&command, 1);
    return read_length == 1 && command == 'S';
}

bool parse_port(const char *text, uint32_t &port)
{
    char *end = nullptr;
    const unsigned long value = std::strtoul(text, &end, 10);
    if(text == end || *end != '\0' || value == 0 || value > 65535)
    {
        return false;
    }
    port = static_cast<uint32_t>(value);
    return true;
}

bool parse_capture_bottom_row(const char *text, int &row)
{
    char *end = nullptr;
    const unsigned long value = std::strtoul(text, &end, 10);
    if(text == end || *end != '\0' || value > SEARCH_UVC_HEIGHT)
    {
        return false;
    }
    row = static_cast<int>(value);
    return true;
}
} // namespace

int main(int argc, char **argv)
{
    if(argc < 4 || argc > 5)
    {
        std::fprintf(stderr,
                     "usage: %s <server_ip> <capture_row_min> <capture_row_max> "
                     "[server_port]\n"
                     "  capture condition: capture_row_min < red bottom row "
                     "< capture_row_max (0-%d in the %dx%d processing image)\n",
                     argv[0],
                     SEARCH_UVC_HEIGHT,
                     SEARCH_UVC_WIDTH,
                     SEARCH_UVC_HEIGHT);
        return 2;
    }

    if(!parse_capture_bottom_row(argv[2], capture_bottom_row_min))
    {
        std::fprintf(stderr,
                     "invalid capture row minimum: %s (expected 0-%d)\n",
                     argv[2],
                     SEARCH_UVC_HEIGHT);
        return 2;
    }
    if(!parse_capture_bottom_row(argv[3], capture_bottom_row_max))
    {
        std::fprintf(stderr,
                     "invalid capture row maximum: %s (expected 0-%d)\n",
                     argv[3],
                     SEARCH_UVC_HEIGHT);
        return 2;
    }
    if(capture_bottom_row_min >= capture_bottom_row_max)
    {
        std::fprintf(stderr,
                     "invalid capture range: minimum %d must be less than maximum %d\n",
                     capture_bottom_row_min,
                     capture_bottom_row_max);
        return 2;
    }

    uint32_t server_port = DEFAULT_SERVER_PORT;
    if(argc == 5 && !parse_port(argv[4], server_port))
    {
        std::fprintf(stderr, "invalid server port: %s\n", argv[4]);
        return 2;
    }

    std::printf("Capturing when %d < red marker bottom row < %d in the "
                "%dx%d processing image.\n",
                capture_bottom_row_min,
                capture_bottom_row_max,
                SEARCH_UVC_WIDTH,
                SEARCH_UVC_HEIGHT);

    std::atexit(cleanup);
    std::signal(SIGINT, sigint_handler);
    std::signal(SIGTERM, sigint_handler);
    std::signal(SIGPIPE, SIG_IGN);
    stop_motors();

    motor_pwm_left.get_dev_info(&motor_pwm_left_info);
    motor_pwm_right.get_dev_info(&motor_pwm_right_info);
    encoder_left.clear_count();
    encoder_right.clear_count();

    if(DEV_NO_FIND == imu_dev.init())
    {
        std::fprintf(stderr, "IMU init failed\n");
        return 1;
    }
    if(0 != uvc_dev.init(UVC_PATH))
    {
        std::fprintf(stderr, "UVC init failed\n");
        return 1;
    }
    if(0 != tcp_client.init(argv[1], server_port))
    {
        std::fprintf(stderr, "TCP connect failed: %s:%u\n", argv[1], server_port);
        return 1;
    }

    std::printf("Keep the car still: calibrating gyro...\n");
    yaw_offset_calibration();
    yaw_degrees.store(0.0f, std::memory_order_relaxed);
    motion_state = MotionState::ROTATE_TO_POSITIVE;
    motion_state_ticks = 0;
    route_encoder_sum = 0;
    move_direction = 1;
    move_target_encoder_sum = MOVE_SEGMENT_ENCODER_COUNT * 2;
    red_bottom_row_report.store(-1, std::memory_order_relaxed);
    gyro_error_0 = 0.0f;
    gyro_error_1 = 0.0f;
    reset_speed_controller(speed_left);
    reset_speed_controller(speed_right);
    motion_state_report.store(static_cast<int>(motion_state), std::memory_order_relaxed);
    route_position_report.store(0, std::memory_order_relaxed);
    std::printf("Calibration complete, offset=%.3f. Starting capture.\n", gyro_z_offset);

    if(0 != pit_timer.init_ms(CONTROL_INTERVAL_MS, control_tick))
    {
        std::fprintf(stderr, "control timer init failed\n");
        return 1;
    }

    uint32_t sequence = 1;
    int last_reported_motion_state = -1;
    auto next_capture = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(CAPTURE_INTERVAL_MS);
    while(true)
    {
        const int reported_motion_state = motion_state_report.load(std::memory_order_relaxed);
        if(reported_motion_state != last_reported_motion_state)
        {
            std::printf("[motion] state=%s, position=%d\n",
                        motion_state_name(reported_motion_state),
                        route_position_report.load(std::memory_order_relaxed));
            last_reported_motion_state = reported_motion_state;
        }

        if(stop_requested())
        {
            std::printf("stop requested by receiver\n");
            break;
        }
        if(uvc_dev.wait_image_refresh() < 0)
        {
            std::fprintf(stderr, "camera capture failed\n");
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if(now < next_capture)
        {
            continue;
        }
        do
        {
            next_capture += std::chrono::milliseconds(CAPTURE_INTERVAL_MS);
        }
        while(next_capture <= now);

        const cv::Mat original_frame = uvc_dev.get_frame_mjpg();
        CaptureRoi capture_roi;
        detect_red_marker(original_frame, capture_roi);
        if(0 != capture_roi.red_found)
        {
            const int bottom_row = capture_roi.red_y + capture_roi.red_h;
            red_bottom_row_report.store(bottom_row,
                                        std::memory_order_relaxed);
            if(bottom_row <= capture_bottom_row_min)
            {
                std::printf("capture skipped: bottom_row=%d <= min %d\n",
                            bottom_row,
                            capture_bottom_row_min);
                continue;
            }
            if(bottom_row >= capture_bottom_row_max)
            {
                std::printf("capture skipped: bottom_row=%d >= max %d\n",
                            bottom_row,
                            capture_bottom_row_max);
                continue;
            }
        }
        else
        {
            red_bottom_row_report.store(-1, std::memory_order_relaxed);
        }
        cv::Mat model_input;
        if(!build_model_input(original_frame, capture_roi, model_input))
        {
            std::printf("capture skipped: no valid 64x64 ROI\n");
            continue;
        }

        if(!send_model_input(model_input, sequence))
        {
            std::fprintf(stderr, "receiver disconnected\n");
            break;
        }
        sequence++;
    }

    return 0;
}
