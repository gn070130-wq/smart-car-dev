#include "car_runtime.hpp"

#include "lq_common.hpp"
#include "lq_display_tft18.hpp"

#ifdef LQ_HAVE_OPENCV
#include <opencv2/core/mat.hpp>
#endif

namespace
{
bool car_runtime_initialized = false;
bool car_runtime_motor_enabled = false;
int car_runtime_motor_init_duty = 1000;
}

void CarRuntime_Init(bool enable_motor, int motor_init_duty)
{
    Data_Settings();
    car_runtime_motor_enabled = enable_motor;
    car_runtime_motor_init_duty = motor_init_duty;

    // 初始化屏幕（竖屏模式）
    lq_tft18_drv_init(1);
    lq_tft18_drv_cls(U16BLACK);

    if (enable_motor)
    {
        Motor_Init1(motor_init_duty);
        Motor_Argument();
    }

    car_runtime_initialized = true;
}

void CarRuntime_Shutdown(bool disable_motor)
{
    if (disable_motor && car_runtime_motor_enabled)
    {
        Motor_Disable1();
    }

    cleanup();
    car_runtime_initialized = false;
}

bool CarRuntime_ProcessFrame(bool enable_motor)
{
    if (!car_runtime_initialized || car_runtime_motor_enabled != enable_motor)
    {
        CarRuntime_Init(enable_motor, car_runtime_motor_init_duty);
    }

    ImageProcess();
    if (enable_motor)
    {
        Motor_Control();
    }

    return true;
}

#ifdef LQ_HAVE_OPENCV
bool CarRuntime_ProcessFrame(const cv::Mat& frame, bool enable_motor)
{
    if (!car_runtime_initialized || car_runtime_motor_enabled != enable_motor)
    {
        CarRuntime_Init(enable_motor, car_runtime_motor_init_duty);
    }

    First_image = frame;
    if (First_image.empty())
    {
        return false;
    }

    ImageProcess();
    if (enable_motor)
    {
        Motor_Control();
    }

    return true;
}
#endif

bool CarRuntime_RunCameraLoop(bool enable_motor,
                              uint16_t width,
                              uint16_t height,
                              uint16_t fps,
                              int motor_init_duty,
                              uint32_t empty_frame_delay_us,
                              uint32_t loop_delay_us)
{
    (void)width;
    (void)height;
    (void)fps;

    CarRuntime_Init(enable_motor, motor_init_duty);

#ifdef LQ_HAVE_OPENCV
    lq_camera cam(width, height, fps);
    if (cam.is_opened())
    {
        while (1)
        {
            if (!CarRuntime_ProcessFrame(cam.get_raw_frame(), enable_motor))
            {
                usleep(empty_frame_delay_us);
                continue;
            }
            usleep(loop_delay_us);
        }
        return true;
    }
    lq_log_info("OpenCV camera not available, falling back to direct image mode");
#endif

    while (1)
    {
        CarRuntime_ProcessFrame(enable_motor);
        usleep(loop_delay_us);
    }

    return true;
}
