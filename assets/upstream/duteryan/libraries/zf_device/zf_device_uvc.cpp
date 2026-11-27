

#include "zf_device_uvc.hpp"
#include "zf_common_headfile.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <sys/select.h>
#include <cerrno>
#include <cstring>
#include <cstdio>

// ==============================================
// 默认曝光配置
// ==============================================
// 建议默认手动曝光，这样更容易稳定高帧率。
// 如果光线太暗，可以适当提高 UVC_DEFAULT_EXPOSURE。
// 如果帧率掉到 60，优先降低曝光值。
int UVC_AUTO_EXPOSURE_MODE = UVC_AUTO_EXPOSURE_DISABLE;
int UVC_DEFAULT_EXPOSURE   = 140;
// 增益:越低噪点越少。固定为低值,防止相机 AGC 为补偿固定曝光而猛拉增益(增益直接放大传感器噪声)。
// 若画面偏暗,优先加曝光/补光,不要靠加这个值。
int UVC_DEFAULT_GAIN       = 0;
// 防频闪:1=50Hz(国内市电,消除日光灯/LED 横向滚动明暗带) 2=60Hz 0=关。
int UVC_POWER_LINE_FREQ    = 1;

// ==============================================
// ioctl 封装：处理 EINTR
// ==============================================
static int xioctl(int fd, unsigned long request, void* arg)
{
    int ret;

    do {
        ret = ioctl(fd, request, arg);
    } while (ret < 0 && errno == EINTR);

    return ret;
}

// ==============================================
// 构造函数
// ==============================================
zf_device_uvc::zf_device_uvc()
    : gray_image(nullptr),
      rgb_image(nullptr),
      v4l2_fd(-1),
      buf_cnt(0),
      tj_dec(nullptr),
      raw_jpeg_size(0),
      is_gray_decoded(false),
      is_bgr_decoded(false),
      is_rgb_converted(false),
      actual_width(UVC_WIDTH),
      actual_height(UVC_HEIGHT),
      uvc_exposure_type(UVC_AUTO_EXPOSURE_DISABLE)
{
    memset(buffers, 0, sizeof(buffers));
    memset(buf_len, 0, sizeof(buf_len));
}

// ==============================================
// 析构函数
// ==============================================
zf_device_uvc::~zf_device_uvc()
{
    if (v4l2_fd >= 0)
    {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(v4l2_fd, VIDIOC_STREAMOFF, &type);

        for (int i = 0; i < buf_cnt; ++i)
        {
            if (buffers[i] && buffers[i] != MAP_FAILED)
            {
                munmap(buffers[i], buf_len[i]);
                buffers[i] = nullptr;
            }
        }

        close(v4l2_fd);
        v4l2_fd = -1;
    }

    if (tj_dec)
    {
        tjDestroy(tj_dec);
        tj_dec = nullptr;
    }
}

// ==============================================
// 设置摄像头帧率
// ==============================================
int8 zf_device_uvc::set_frame_rate(int fps)
{
    if (v4l2_fd < 0 || fps <= 0) {
        return -1;
    }

    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = fps;

    if (xioctl(v4l2_fd, VIDIOC_S_PARM, &parm) < 0)
    {
        std::cerr << "[UVC] 设置帧率失败，驱动可能不支持 VIDIOC_S_PARM" << std::endl;
        return -1;
    }

    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(v4l2_fd, VIDIOC_G_PARM, &parm) == 0)
    {
        int num = parm.parm.capture.timeperframe.numerator;
        int den = parm.parm.capture.timeperframe.denominator;

        if (num > 0 && den > 0)
        {
            double real_fps = (double)den / (double)num;

            std::cout << "[UVC] 请求帧率: " << fps
                      << " FPS | 实际帧率: " << real_fps
                      << " FPS" << std::endl;
        }
    }

    return 0;
}

// ==============================================
// 打印当前实际格式
// ==============================================
void zf_device_uvc::print_current_format_info()
{
    if (v4l2_fd < 0) {
        return;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(v4l2_fd, VIDIOC_G_FMT, &fmt) == 0)
    {
        actual_width  = fmt.fmt.pix.width;
        actual_height = fmt.fmt.pix.height;

        char fourcc[5] = {0};
        fourcc[0] = fmt.fmt.pix.pixelformat & 0xFF;
        fourcc[1] = (fmt.fmt.pix.pixelformat >> 8) & 0xFF;
        fourcc[2] = (fmt.fmt.pix.pixelformat >> 16) & 0xFF;
        fourcc[3] = (fmt.fmt.pix.pixelformat >> 24) & 0xFF;

        std::cout << "[UVC] 实际格式: "
                  << actual_width << "x" << actual_height
                  << " fourcc=" << fourcc
                  << " sizeimage=" << fmt.fmt.pix.sizeimage
                  << std::endl;
    }
}

// ==============================================
// 初始化
// ==============================================
int8 zf_device_uvc::init(const char *path)
{
    // 1. 打开摄像头
    v4l2_fd = open(path, O_RDWR | O_NONBLOCK);
    if (v4l2_fd < 0)
    {
        std::cerr << "[UVC] 设备打开失败: " << path << std::endl;
        return -1;
    }

    // 2. 初始化 TurboJPEG
    tj_dec = tjInitDecompress();
    if (!tj_dec)
    {
        std::cerr << "[UVC] TurboJPEG 解码器初始化失败" << std::endl;
        close(v4l2_fd);
        v4l2_fd = -1;
        return -1;
    }

    // 3. 设置视频格式：MJPEG
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = UVC_WIDTH;
    fmt.fmt.pix.height      = UVC_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;

    if (xioctl(v4l2_fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        std::cerr << "[UVC] 设置视频格式失败" << std::endl;
        close(v4l2_fd);
        v4l2_fd = -1;
        return -1;
    }

    print_current_format_info();

    // 4. 显式设置 FPS
    set_frame_rate(UVC_FPS);

    // 5. 设置曝光
    // 注意：放在 STREAMON 前面，减少启动阶段状态抖动。
    if (this->set_auto_exposure(UVC_AUTO_EXPOSURE_MODE) == 0)
    {
        if (UVC_AUTO_EXPOSURE_MODE == UVC_AUTO_EXPOSURE_DISABLE)
        {
            this->set_exposure_value(UVC_DEFAULT_EXPOSURE);

            std::cout << "[UVC] 曝光模式: 手动 | 曝光值: "
                      << UVC_DEFAULT_EXPOSURE << std::endl;
        }
        else
        {
            std::cout << "[UVC] 曝光模式: 自动" << std::endl;
        }
    }
    else
    {
        std::cerr << "[UVC] 警告: 无法设置曝光模式" << std::endl;
    }

    // 6. 设置增益(降噪):固定为低增益,防止 AGC 猛拉放大噪点
    if (this->set_gain(UVC_DEFAULT_GAIN) == 0)
        std::cout << "[UVC] 增益: " << UVC_DEFAULT_GAIN << " (越低噪点越少)" << std::endl;
    else
        std::cerr << "[UVC] 警告: 无法设置增益(驱动可能不支持 V4L2_CID_GAIN)" << std::endl;

    // 7. 设置防频闪(国内 50Hz):消除灯光下横向滚动明暗带
    if (this->set_power_line_frequency(UVC_POWER_LINE_FREQ) == 0)
        std::cout << "[UVC] 防频闪: " << UVC_POWER_LINE_FREQ << " (1=50Hz/2=60Hz/0=关)" << std::endl;
    else
        std::cerr << "[UVC] 警告: 无法设置防频闪(驱动可能不支持 V4L2_CID_POWER_LINE_FREQUENCY)" << std::endl;

    // 6. 申请 MMAP 缓冲区
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(v4l2_fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2)
    {
        std::cerr << "[UVC] 申请内核缓冲区失败" << std::endl;
        close(v4l2_fd);
        v4l2_fd = -1;
        return -1;
    }

    buf_cnt = req.count;
    if (buf_cnt > 4) {
        buf_cnt = 4;
    }

    // 7. mmap
    for (int i = 0; i < buf_cnt; ++i)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (xioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf) < 0)
        {
            std::cerr << "[UVC] VIDIOC_QUERYBUF 失败" << std::endl;
            close(v4l2_fd);
            v4l2_fd = -1;
            return -1;
        }

        buffers[i] = mmap(NULL,
                          buf.length,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          v4l2_fd,
                          buf.m.offset);

        if (buffers[i] == MAP_FAILED)
        {
            std::cerr << "[UVC] mmap 失败" << std::endl;
            close(v4l2_fd);
            v4l2_fd = -1;
            return -1;
        }

        buf_len[i] = buf.length;
    }

    // 8. 所有 buffer 入队
    for (int i = 0; i < buf_cnt; ++i)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (xioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0)
        {
            std::cerr << "[UVC] VIDIOC_QBUF 失败" << std::endl;
            close(v4l2_fd);
            v4l2_fd = -1;
            return -1;
        }
    }

    // 9. 预分配缓存
    frame_mjpg.create(actual_height, actual_width, CV_8UC3);
    frame_gray.create(actual_height, actual_width, CV_8UC1);
    frame_rgb.create(actual_height, actual_width, CV_8UC2);

    raw_jpeg_buffer.reserve(256 * 1024);
    raw_jpeg_size = 0;

    is_gray_decoded  = false;
    is_bgr_decoded   = false;
    is_rgb_converted = false;

    // 10. 开启视频流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(v4l2_fd, VIDIOC_STREAMON, &type) < 0)
    {
        std::cerr << "[UVC] 开启视频采集流失败" << std::endl;
        close(v4l2_fd);
        v4l2_fd = -1;
        return -1;
    }

    // 11. 丢弃启动前几帧，让曝光/白平衡/码流稳定
    discard_startup_frames(8);

    std::cout << "[UVC] 初始化成功 "
              << actual_width << "x" << actual_height
              << " MJPEG | 目标FPS=" << UVC_FPS
              << " | 缓存懒加载模式" << std::endl;

    return 0;
}

// ==============================================
// 丢弃启动前几帧
// ==============================================
void zf_device_uvc::discard_startup_frames(int count)
{
    for (int i = 0; i < count; ++i) {
        wait_image_refresh();
    }

    is_gray_decoded  = false;
    is_bgr_decoded   = false;
    is_rgb_converted = false;
}

// ==============================================
// 等待并获取一帧 MJPEG 原始数据
// ==============================================
int8 zf_device_uvc::wait_image_refresh()
{
    if (v4l2_fd < 0 || !tj_dec) {
        return -1;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(v4l2_fd, &fds);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int ret = select(v4l2_fd + 1, &fds, NULL, NULL, &tv);
    if (ret <= 0) {
        return -1;
    }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));

    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(v4l2_fd, VIDIOC_DQBUF, &buf) < 0)
    {
        return -1;
    }

    // ==========================================================
    // 关键修复：
    // 每帧都必须 resize 到当前 bytesused。
    // 否则当前帧比上一帧小的时候，会把上一帧尾部残留也交给 TurboJPEG。
    // ==========================================================
    raw_jpeg_buffer.resize(buf.bytesused);
    raw_jpeg_size = buf.bytesused;

    memcpy(raw_jpeg_buffer.data(), buffers[buf.index], buf.bytesused);

    // 尽快还给驱动，避免采集队列阻塞
    if (xioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0)
    {
        return -1;
    }

    // 新帧到来，清空当前帧懒加载状态
    is_gray_decoded  = false;
    is_bgr_decoded   = false;
    is_rgb_converted = false;

    // 调试时可以打开，观察码流大小是否抖动。
    /*
    static int cnt = 0;
    if (++cnt % 60 == 0) {
        printf("[UVC] bytesused=%u\n", buf.bytesused);
    }
    */

    return 0;
}

// ==============================================
// 获取灰度图。每帧最多解码一次灰度。
// ==============================================
uint8_t* zf_device_uvc::get_gray_image_ptr()
{
    if (!is_gray_decoded && !raw_jpeg_buffer.empty() && raw_jpeg_size > 0)
    {
        int w = 0;
        int h = 0;
        int subsamp = 0;
        int cs = 0;

        if (tjDecompressHeader3(tj_dec,
                                raw_jpeg_buffer.data(),
                                (unsigned long)raw_jpeg_size,
                                &w, &h,
                                &subsamp,
                                &cs) == 0)
        {
            if (w != frame_gray.cols || h != frame_gray.rows)
            {
                frame_gray.create(h, w, CV_8UC1);
                actual_width = w;
                actual_height = h;
            }

            int ret = tjDecompress2(tj_dec,
                                    raw_jpeg_buffer.data(),
                                    (unsigned long)raw_jpeg_size,
                                    frame_gray.data,
                                    w,
                                    (int)frame_gray.step,
                                    h,
                                    TJPF_GRAY,
                                    TJFLAG_ACCURATEDCT);  // 精确IDCT:比FASTDCT少块状/振铃伪影(降噪)

            if (ret == 0) {
                is_gray_decoded = true;
            }
        }
    }

    gray_image = frame_gray.ptr<uint8_t>(0);
    return gray_image;
}
/// ==============================================
// 获取 RGB565 图像数据指针 (用于 LCD 实时显示)
// 采用懒加载：每帧最多执行一次彩色解码与转换
// ==============================================
uint16_t* zf_device_uvc::get_rgb565_image_ptr()
{
    // 检查是否已经转换过，以及原始 JPEG 码流是否有效
    if (!is_rgb_converted && !raw_jpeg_buffer.empty() && raw_jpeg_size > 0)
    {
        int w = 0, h = 0, subsamp = 0, cs = 0;

        // 1. 解析 JPEG 头部获取分辨率
        if (tjDecompressHeader3(tj_dec,
                                raw_jpeg_buffer.data(),
                                (unsigned long)raw_jpeg_size,
                                &w, &h,
                                &subsamp,
                                &cs) == 0)
        {
            // 如果分辨率发生变化，重新分配 OpenCV 的 Mat 内存空间
            if (w != frame_mjpg.cols || h != frame_mjpg.rows)
            {
                frame_mjpg.create(h, w, CV_8UC3); // 存放解压出的 BGR 数据
                frame_rgb.create(h, w, CV_8UC2);  // 存放转换后的 RGB565 数据
                actual_width = w;
                actual_height = h;
            }

            // 2. 利用 TurboJPEG 硬件/指令集加速，将 MJPEG 解码为 BGR 格式
            int ret = tjDecompress2(tj_dec,
                                    raw_jpeg_buffer.data(),
                                    (unsigned long)raw_jpeg_size,
                                    frame_mjpg.data,
                                    w,
                                    (int)frame_mjpg.step,
                                    h,
                                    TJPF_BGR,
                                    TJFLAG_ACCURATEDCT); // 精确IDCT:比FASTDCT少块状/振铃伪影(降噪)

            if (ret == 0) 
            {
                // 3. 将BGR打包为真正的RGB565，专供LCD刷屏使用。
                // OpenCV的COLOR_BGR2BGR565会生成BGR565，高5位是B；IPS200按RGB565显示会红蓝互换。
                for (int y = 0; y < h; y++)
                {
                    const cv::Vec3b* src = frame_mjpg.ptr<cv::Vec3b>(y);
                    uint16_t* dst = frame_rgb.ptr<uint16_t>(y);
                    for (int x = 0; x < w; x++)
                    {
                        uint16_t b = src[x][0];
                        uint16_t g = src[x][1];
                        uint16_t r = src[x][2];
                        dst[x] = static_cast<uint16_t>(((r & 0xF8) << 8) |
                                                       ((g & 0xFC) << 3) |
                                                       ( b >> 3));
                    }
                }
                
                is_rgb_converted = true; // 标记本帧已完成转换，避免重复消耗 CPU
            }
        }
    }

    // 4. 返回内部数据的裸指针
    rgb_image = reinterpret_cast<uint16_t*>(frame_rgb.data);
    return rgb_image;
}
// ==============================================
// 获取当前帧 BGR 图。每帧最多解码一次 BGR。
// ==============================================
cv::Mat zf_device_uvc::get_frame_mjpg()
{
    if (!is_bgr_decoded && !raw_jpeg_buffer.empty() && raw_jpeg_size > 0)
    {
        int w = 0;
        int h = 0;
        int subsamp = 0;
        int cs = 0;

        if (tjDecompressHeader3(tj_dec,
                                raw_jpeg_buffer.data(),
                                (unsigned long)raw_jpeg_size,
                                &w, &h,
                                &subsamp,
                                &cs) != 0)
        {
            return cv::Mat();
        }

        if (w != frame_mjpg.cols || h != frame_mjpg.rows)
        {
            frame_mjpg.create(h, w, CV_8UC3);
            actual_width = w;
            actual_height = h;
        }

        int ret = tjDecompress2(tj_dec,
                                raw_jpeg_buffer.data(),
                                (unsigned long)raw_jpeg_size,
                                frame_mjpg.data,
                                w,
                                (int)frame_mjpg.step,
                                h,
                                TJPF_BGR,
                                TJFLAG_FASTDCT);

        if (ret != 0) {
            return cv::Mat();
        }

        is_bgr_decoded = true;
    }

    return frame_mjpg;
}

// ==============================================
// 获取当前帧 BGR ROI
// 重要：
// 1. 不再 tjTransform
// 2. 不再解小 JPEG
// 3. 不再 clone
// 4. 返回的是 frame_mjpg 的 ROI 视图
// ==============================================
cv::Mat zf_device_uvc::get_frame_mjpg_roi(int crop_x1, int crop_y1, int crop_x2, int crop_y2)
{
    cv::Mat bgr = get_frame_mjpg();

    if (bgr.empty()) {
        return cv::Mat();
    }

    if (crop_x1 < 0) crop_x1 = 0;
    if (crop_y1 < 0) crop_y1 = 0;
    if (crop_x2 >= bgr.cols) crop_x2 = bgr.cols - 1;
    if (crop_y2 >= bgr.rows) crop_y2 = bgr.rows - 1;

    int w = crop_x2 - crop_x1 + 1;
    int h = crop_y2 - crop_y1 + 1;

    if (w <= 0 || h <= 0) {
        return cv::Mat();
    }

    cv::Rect rect(crop_x1, crop_y1, w, h);

    return bgr(rect);
}

// ==============================================
// 摄像头状态
// ==============================================
bool zf_device_uvc::is_camera_opened() const
{
    return v4l2_fd >= 0;
}

// ==============================================
// 设置曝光模式
// ==============================================
int8 zf_device_uvc::set_auto_exposure(int32_t mode)
{
    if (v4l2_fd < 0) {
        return -1;
    }

    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));

    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ctrl.value = mode;

    if (xioctl(v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
        return -1;
    }

    uvc_exposure_type = (uvc_exposure_type_enum)mode;
    return 0;
}

// ==============================================
// 设置曝光值
// ==============================================
int8 zf_device_uvc::set_exposure_value(int32_t val)
{
    if (v4l2_fd < 0) {
        return -1;
    }

    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));

    ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    ctrl.value = val;

    return xioctl(v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0 ? -1 : 0;
}

// ==============================================
// 设置增益(越低噪点越少;固定它防止 AGC 猛拉放大噪声)
// ==============================================
int8 zf_device_uvc::set_gain(int32_t gain_value)
{
    if (v4l2_fd < 0) {
        return -1;
    }

    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));

    ctrl.id = V4L2_CID_GAIN;
    ctrl.value = gain_value;

    return xioctl(v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0 ? -1 : 0;
}

// ==============================================
// 设置防频闪频率(1=50Hz 2=60Hz 0=关),消除灯光横向滚动条纹
// ==============================================
int8 zf_device_uvc::set_power_line_frequency(int32_t freq)
{
    if (v4l2_fd < 0) {
        return -1;
    }

    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));

    ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
    ctrl.value = freq;

    return xioctl(v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0 ? -1 : 0;
}

// ==============================================
// 查询曝光模式
// ==============================================
double zf_device_uvc::get_auto_exposure_mode()
{
    if (v4l2_fd < 0) {
        return -1;
    }

    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));

    ctrl.id = V4L2_CID_EXPOSURE_AUTO;

    if (xioctl(v4l2_fd, VIDIOC_G_CTRL, &ctrl) < 0) {
        return -1;
    }

    return ctrl.value;
}

// ==============================================
// 查询曝光值
// ==============================================
double zf_device_uvc::get_current_exposure()
{
    if (v4l2_fd < 0) {
        return -1;
    }

    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));

    ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;

    if (xioctl(v4l2_fd, VIDIOC_G_CTRL, &ctrl) < 0) {
        return -1;
    }

    return ctrl.value;
}
