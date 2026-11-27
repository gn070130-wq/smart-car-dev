#include "camera.h"
#include <sys/resource.h>
#include <chrono>
#include <thread>
#include <sys/time.h>
#include <time.h>
#include <turbojpeg.h>
#include "my_timeer.h"  
#include "rotate_array.h"
#include "json.hpp"
#include <mutex>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef V4L2_CAP_IO_MC
#define V4L2_CAP_IO_MC 0x20000000
#endif

using json = nlohmann::json;

int rows;
int cols;
uchar *p;
Mat frame;
Mat grayImage;
Mat binaryImage;
Mat adjusted;
Mat g_yuyv;                    // YUYV 原始图像 (CV_8UC2)，彩色逆透视基于此
Mat square;
Mat grayImage_raw;
uint8_t *PerImg_ip_1[RESULT_ROW][RESULT_COL];
uint8_t *PerImg_ip[RESULT_ROW][RESULT_COL];
int exposure = 100;
int auto_exposure = 1;
float image_scale = 1.0;

static const char *CAMERA_CFG_PATH = "/home/root/configs/camera_config.json";
static std::mutex g_camera_cfg_mutex;
static const double DEFAULT_IPM_MAT[3][3] = {
    {-0.373500, 0.407359, -72.087307},
    {-0.000000, 0.071048, -51.245684},
    {-0.000000, 0.002520, -0.817953}
};
double camera_ipm_mat[3][3] = {
    {-0.373500, 0.407359, -72.087307},
    {-0.000000, 0.071048, -51.245684},
    {-0.000000, 0.002520, -0.817953}
};
double camera_ipm_pitch = 0.002520; // ImagePerspective2 分母 y 项，控制远近横向缩放/俯仰透视强度
double camera_ipm_scale = 1.0;      // 逆透视结果整体缩放，1.0 为原始大小
int camera_ipm_offset_x = 0;        // 逆透视结果整体水平平移（输出像素）
int camera_ipm_offset_y = 0;        // 逆透视结果整体垂直平移（输出像素）

static void transform_ipm_output(double x, double y, double width, double height,
                                 double offset_scale, double &mapped_x, double &mapped_y)
{
    const double center_x = (width - 1.0) * 0.5;
    const double center_y = (height - 1.0) * 0.5;
    mapped_x = center_x + (x - center_x) / camera_ipm_scale
             - camera_ipm_offset_x * offset_scale;
    mapped_y = center_y + (y - center_y) / camera_ipm_scale
             - camera_ipm_offset_y * offset_scale;
}

static double calc_ipm_local(const double mat[3][3], int row, double x, double y)
{
    double den = mat[2][0] * x + mat[2][1] * y + mat[2][2];
    return (mat[row][0] * x + mat[row][1] * y + mat[row][2]) / den;
}

void camera_ipm_set_pitch(double pitch)
{
    // 单独改变 M[2][1] 会同时改变 local_x/local_y，造成图像横向、纵向漂移。
    // 用默认矩阵的底部中心映射作为锚点，同步补偿 M[0][1] 和 M[1][1]：
    // M[0][1] 固定横向位置，M[1][1] 固定纵向位置，只保留远近宽度比例变化。
    const double anchor_x = (RESULT_COL - 1) * 0.5;
    const double anchor_y = RESULT_ROW - 1;
    const double target_x = calc_ipm_local(DEFAULT_IPM_MAT, 0, anchor_x, anchor_y);
    const double target_y = calc_ipm_local(DEFAULT_IPM_MAT, 1, anchor_x, anchor_y);

    camera_ipm_pitch = pitch;
    camera_ipm_mat[2][1] = camera_ipm_pitch;

    double den = camera_ipm_mat[2][0] * anchor_x + camera_ipm_mat[2][1] * anchor_y + camera_ipm_mat[2][2];
    if (anchor_y != 0.0)
    {
        camera_ipm_mat[0][1] =
            (target_x * den - camera_ipm_mat[0][0] * anchor_x - camera_ipm_mat[0][2]) / anchor_y;
        camera_ipm_mat[1][1] =
            (target_y * den - camera_ipm_mat[1][0] * anchor_x - camera_ipm_mat[1][2]) / anchor_y;
    }
}

extern float angle_barrier_save;

extern int size_frame;

void camera_cfg_save()
{
    mkdir("/home/root/configs", 0777);
    std::lock_guard<std::mutex> cfg_lock(g_camera_cfg_mutex);
    json j = json::object();
    try {
        std::ifstream existing(CAMERA_CFG_PATH);
        if (existing.is_open()) existing >> j;
        if (!j.is_object()) j = json::object();
    } catch (...) { j = json::object(); }
    j["ipm_pitch"] = camera_ipm_pitch;
    j["ipm_scale"] = camera_ipm_scale;
    j["ipm_offset_x"] = camera_ipm_offset_x;
    j["ipm_offset_y"] = camera_ipm_offset_y;
    j["change_un_Mat"] = json::array();
    for (int r = 0; r < 3; ++r)
    {
        json row = json::array();
        for (int c = 0; c < 3; ++c) row.push_back(camera_ipm_mat[r][c]);
        j["change_un_Mat"].push_back(row);
    }

    const std::string temp_path = std::string(CAMERA_CFG_PATH) + ".tmp";
    std::ofstream outfile(temp_path);
    if (outfile.is_open())
    {
        outfile << j.dump(4);
        outfile.close();
        if (rename(temp_path.c_str(), CAMERA_CFG_PATH) != 0) unlink(temp_path.c_str());
    }
}

void camera_cfg_load()
{
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            camera_ipm_mat[r][c] = DEFAULT_IPM_MAT[r][c];
    camera_ipm_pitch = DEFAULT_IPM_MAT[2][1];
    camera_ipm_scale = 1.0;
    camera_ipm_offset_x = 0;
    camera_ipm_offset_y = 0;

    std::ifstream infile(CAMERA_CFG_PATH);
    if (!infile.is_open())
    {
        camera_cfg_save();
        return;
    }

    try
    {
        json j;
        infile >> j;
        if (j.contains("change_un_Mat") && j["change_un_Mat"].is_array())
        {
            for (int r = 0; r < 3 && r < (int)j["change_un_Mat"].size(); ++r)
            {
                if (!j["change_un_Mat"][r].is_array()) continue;
                for (int c = 0; c < 3 && c < (int)j["change_un_Mat"][r].size(); ++c)
                    camera_ipm_mat[r][c] = j["change_un_Mat"][r][c].get<double>();
            }
        }
        camera_ipm_set_pitch(j.value("ipm_pitch", camera_ipm_mat[2][1]));
        camera_ipm_scale = j.value("ipm_scale", 1.0);
        camera_ipm_offset_x = j.value("ipm_offset_x", 0);
        camera_ipm_offset_y = j.value("ipm_offset_y", 0);
        if (camera_ipm_scale < 0.5 || camera_ipm_scale > 2.0)
            camera_ipm_scale = 1.0;
    }
    catch (const std::exception &e)
    {
        cerr << "Camera config load failed, using hardcoded defaults: " << e.what() << endl;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                camera_ipm_mat[r][c] = DEFAULT_IPM_MAT[r][c];
        camera_ipm_set_pitch(DEFAULT_IPM_MAT[2][1]);
    }
    infile.close();
}

static bool camera_supports_format(int fd, uint32_t pixel_format)
{
    struct v4l2_fmtdesc format = {0};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &format) == 0)
    {
        if (format.pixelformat == pixel_format) return true;
        ++format.index;
    }
    return false;
}

static std::string find_camera_device()
{
    char dev_name[16];
    for (int i = 0; i < 64; ++i)
    { // 通常遍历前64个节点足够了
        snprintf(dev_name, sizeof(dev_name), "/dev/video%d", i);

        int fd = open(dev_name, O_RDWR);
        if (fd < 0)
            continue;

        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) >= 0)
        {
            // 检查是否支持视频采集功能 (V4L2_CAP_VIDEO_CAPTURE)
            // 且排除掉一些只有元数据或输出功能的伪节点
            uint32_t caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS)
                              ? cap.device_caps : cap.capabilities;
            if ((caps & V4L2_CAP_VIDEO_CAPTURE) &&
                (caps & V4L2_CAP_STREAMING) &&
                !(caps & V4L2_CAP_VIDEO_OUTPUT) &&
                !(caps & V4L2_CAP_IO_MC) &&
                camera_supports_format(fd, V4L2_PIX_FMT_YUYV))
            {
                close(fd);
                return std::string(dev_name);
            }
        }
        close(fd);
    }
    return "";
}

void camera_init()
{
    std::string dev_path = find_camera_device();
    if (dev_path.empty())
    {
        cerr << "Error: No usable USB camera found!" << endl;
        return;
    }

    // 从设备路径中提取索引数字 (例如从 "/dev/video2" 提取 2)
    int index = std::stoi(dev_path.substr(10));

    if (!cap.open(index))
    { // 使用自动找到的索引打开
        cerr << "Error: VideoCapture failed to open " << dev_path << endl;
        return;
    }

    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap.set(CAP_PROP_FRAME_WIDTH, 320);
    cap.set(CAP_PROP_FRAME_HEIGHT, 240);
    cap.set(CAP_PROP_FPS, 156);
    cap.set(CAP_PROP_BUFFERSIZE, 2);

    camera_cfg_load();
    get_image(1);
    ImagePerspective1_Init();
    ImagePerspective2_Init();
    get_image(3);
    ImagePerspective1_Color_Init();
    ImagePerspective2_Color_Init();

    cout << "VideoCapture initialized on: " << dev_path << endl;
}

void camera_exposure_setting()
{
    cap.set(CAP_PROP_AUTO_EXPOSURE, auto_exposure);
    cap.set(CAP_PROP_EXPOSURE, exposure);
}

void get_camera_message()
{
    double frameWidth = cap.get(CAP_PROP_FRAME_WIDTH);
    double frameHeight = cap.get(CAP_PROP_FRAME_HEIGHT);
    double fps = cap.get(CAP_PROP_FPS);
    double fourcc = cap.get(CAP_PROP_FOURCC);
    double buffersize = cap.get(CAP_PROP_BUFFERSIZE);
    double contrast = cap.get(CAP_PROP_CONTRAST);
    double sharpness = cap.get(CAP_PROP_SHARPNESS);
    double exposure = cap.get(CAP_PROP_EXPOSURE);
    double auto_exposure = cap.get(CAP_PROP_AUTO_EXPOSURE);

    std::cout << "Camera Parameters:" << std::endl;
    std::cout << "Frame Width:   " << frameWidth << std::endl;
    std::cout << "Frame Height:  " << frameHeight << std::endl;
    std::cout << "FPS:           " << fps << std::endl;
    std::cout << "FOURCC:        " << (char)((int)fourcc & 0xFF)
              << (char)(((int)fourcc >> 8) & 0xFF)
              << (char)(((int)fourcc >> 16) & 0xFF)
              << (char)(((int)fourcc >> 24) & 0xFF)
              << std::endl;
    std::cout << "Buffer Size:   " << buffersize << std::endl;
    std::cout << "Contrast:      " << contrast << std::endl;
    std::cout << "Sharpness:     " << sharpness << std::endl;
    std::cout << "Exposure:      " << exposure << std::endl;
    std::cout << "Auto Exposure: " << auto_exposure << std::endl;
}
float calcOtsuFromHist(const int hist[256], int sample_total)
{
    int min_t = 0;
    while (min_t < 256 && hist[min_t] == 0) min_t++;
    
    int max_t = 255;
    while (max_t > min_t && hist[max_t] == 0) max_t--;
    
    if (min_t >= max_t) return (float)min_t;

    float sum = 0.0f;
    for (int t = min_t; t <= max_t; ++t)
    {
        sum += (float)t * hist[t];
    }

    float sumB = 0.0f;
    int wB = 0;
    float varMax = -1.0f;
    float threshold = (float)min_t;

    for (int t = min_t; t < max_t; ++t)
    {
        wB += hist[t];
        sumB += (float)t * hist[t];

        int wF = sample_total - wB;
        
        float compare_val = sumB * (float)sample_total - sum * (float)wB;
        float varBetween = (compare_val * compare_val) / ((float)wB * (float)wF);

        if (varBetween > varMax)
        {
            varMax = varBetween;
            threshold = (float)t;
        }
    }
    return threshold;
}

// 核心业务函数：双重迭代大津法
float getOtsuThreshold(const cv::Mat &grayImage)
{
    CV_Assert(grayImage.type() == CV_8UC1);

    int hist[256] = {0};
    int sample_total = 0;

    int rows = grayImage.rows;
    int cols = grayImage.cols;
    
    // 1. 统计原始直方图（全图仅遍历这一次内存）
    if (grayImage.isContinuous())
    {
        const uchar* data = grayImage.ptr<uchar>(0);
        for (int y = 0; y < rows; y += 1)
        {
            const uchar* row_data = data + y * cols;
            for (int x = 0; x < cols; x += 1)
            {
                ++hist[row_data[x]];
                ++sample_total; // 保持在这里统计，后面原地重组时总数不变
            }
        }
    }
    else
    {
        for (int y = 0; y < rows; y += 1)
        {
            const uchar *row = grayImage.ptr<uchar>(y);
            for (int x = 0; x < cols; x += 1)
            {
                ++hist[row[x]];
                ++sample_total;
            }
        }
    }

    // 2. 第一次大津法：计算基础阈值 T1
    float T1 = calcOtsuFromHist(hist, sample_total);
    int idxT1 = (int)T1;

    // 3. 核心优化：原地重组直方图
    // 模拟“把所有低于 T1 的像素强行拉高到 T1”的效果
    // 将 0 ~ T1-1 的像素频数全部累加到 T1 位置上，并将低位清零
    int low_sum = 0;
    for (int i = 0; i < idxT1; ++i)
    {
        low_sum += hist[i];
        hist[i] = 0; // 清零，确保 calcOtsuFromHist 内部缩减边界逻辑正确
    }
    hist[idxT1] += low_sum; 

    // 4. 第二次大津法：直接传入重组后的直方图，计算最终科学提高后的阈值 T2
    // 由于 sample_total 在重组前后没有发生发生改变，直接复用
    float T2 = calcOtsuFromHist(hist, sample_total);

    return T2;
}

float getStandardOtsuThreshold(const cv::Mat &grayImage, float scale = 1.0f)
{
    CV_Assert(grayImage.type() == CV_8UC1);

    int hist[256] = {0};
    int sample_total = 0;

    int rows = grayImage.rows;
    int cols = grayImage.cols;
    
    // 1. 统计直方图（高效遍历内存）
    if (grayImage.isContinuous())
    {
        const uchar* data = grayImage.ptr<uchar>(0);
        int total_pixels = rows * cols;
        for (int i = 0; i < total_pixels; ++i)
        {
            ++hist[data[i]];
        }
        sample_total = total_pixels;
    }
    else
    {
        for (int y = 0; y < rows; ++y)
        {
            const uchar *row = grayImage.ptr<uchar>(y);
            for (int x = 0; x < cols; ++x)
            {
                ++hist[row[x]];
                ++sample_total;
            }
        }
    }

    // 2. 标准大津法核心计算（通过增量更新优化类间方差计算）
    double sum_total = 0;
    for (int i = 0; i < 256; ++i)
    {
        sum_total += i * hist[i]; // 全局总灰度值
    }

    double sum_back = 0;          // 背景总灰度值
    long long w_back = 0;         // 背景像素总数
    long long w_fore = 0;         // 前景像素总数

    double max_variance = 0.0;    // 最大类间方差
    int best_threshold = 0;       // 最佳阈值

    for (int i = 0; i < 256; ++i)
    {
        w_back += hist[i];
        if (w_back == 0) continue;
        
        w_fore = sample_total - w_back;
        if (w_fore == 0) break; // 后面没有像素了，提前结束

        sum_back += (double)(i * hist[i]);
        double sum_fore = sum_total - sum_back;

        // 背景和前景的平均灰度
        double mu_back = sum_back / w_back;
        double mu_fore = sum_fore / w_fore;

        // 计算类间方差：w_back * w_fore * (mu_back - mu_fore)^2
        // 为了防止大图下像素乘积溢出，转换为 double 计算
        double variance = (double)w_back * (double)w_fore * (mu_back - mu_fore) * (mu_back - mu_fore);

        // 寻找最大方差对应的灰度值
        if (variance > max_variance)
        {
            max_variance = variance;
            best_threshold = i;
        }
    }

    // 3. 返回计算出的阈值 * 比例参数
    return (float)best_threshold * scale;
}

int get_image(int setting)
{

    try
    {

        cap >> frame;
        // flip(frame, frame, 1);
        // flip(frame, frame, 0);

        if (frame.empty())
        {
            cerr << "Can not get image!" << std::endl;
            return -1;
        }
    }
    catch (const cv::Exception &e)
    {
        cerr << "OpenCV error!: " << e.what() << std::endl;
        return -1;
    }

    // 二值化
    if (setting == 1)
    {
        // frame = imread("/home/root/otsu.png");
        cvtColor(frame, grayImage, cv::COLOR_BGR2GRAY);
        float threshold_num = getOtsuThreshold(grayImage);

        // 2. 传入你算好的 threshold_num，注意：必须去掉 | THRESH_OTSU 标记！
        threshold(adjusted, binaryImage, threshold_num, 255, THRESH_BINARY);
        rows = binaryImage.rows;
        cols = binaryImage.cols;
        p = binaryImage.data;
        return 1;
    }
    // 灰度图
    else if (setting == 2)
    {

        cvtColor(frame, grayImage, COLOR_BGR2GRAY);
        rows = grayImage.rows;
        cols = grayImage.cols;
        p = grayImage.data;
        return 1;
    }
    // 彩色
    else if (setting == 3)
    {

        rows = frame.rows;
        cols = frame.cols;
        p = frame.data;
        return 1;
    }
    return 0;
}

void ImagePerspective1_Init()
{

    static uint8_t BlackColor = 125;
    

    double cameraMatrix[3][3] = {{92.055875, 0.000000, 166.461189}, {0.000000, 92.320304, 115.837917}, {0.000000, 0.000000, 1.000000}};
    double distCoeffs[5] = {-0.014048, -0.013327, -0.001824, -0.001394, 0.002692};
    int move_xy[2] = {0, 0};

    


    double fx = cameraMatrix[0][0], fy = cameraMatrix[1][1], ux = cameraMatrix[0][2], uy = cameraMatrix[1][2], k1 = distCoeffs[0], k2 = distCoeffs[1], k3 = distCoeffs[4], p1 = distCoeffs[2], p2 = distCoeffs[3];

    int move_x = move_xy[0], move_y = move_xy[1];

    for (int i = -move_y; i < RESULT_ROW - move_y; i++)
    {
        for (int j = -move_x; j < RESULT_COL - move_x; j++)
        {
            double xCorrected = (j - ux) / fx;
            double yCorrected = (i - uy) / fy;
            double xDistortion, yDistortion;
            double r2 = xCorrected * xCorrected + yCorrected * yCorrected;
            double deltaRa = 1. + k1 * r2 + k2 * r2 * r2 + k3 * r2 * r2 * r2;
            double deltaRb = 1 / (1.);
            double deltaTx = 2. * p1 * xCorrected * yCorrected + p2 * (r2 + 2. * xCorrected * xCorrected);
            double deltaTy = p1 * (r2 + 2. * yCorrected * yCorrected) + 2. * p2 * xCorrected * yCorrected;
            xDistortion = xCorrected * deltaRa * deltaRb + deltaTx;
            yDistortion = yCorrected * deltaRa * deltaRb + deltaTy;
            xDistortion = xDistortion * fx + ux;
            yDistortion = yDistortion * fy + uy;

            if (i + move_y >= 0 && i + move_y < RESULT_ROW && j + move_x >= 0 && j + move_x < RESULT_COL)
            {
                if (yDistortion >= 0 && yDistortion < USED_ROW && xDistortion >= 0 && xDistortion < USED_COL)
                {
                    PerImg_ip_1[i + move_y][j + move_x] = &p[(int)yDistortion * USED_COL + (int)xDistortion];
                }
                else
                {
                    PerImg_ip_1[i + move_y][j + move_x] = &BlackColor;
                }
            }
        }
    }
}

void ImagePerspective2_Init()
{
    static uint8_t BlackColor = 125;
    //double change_un_Mat[3][3] ={{-0.368950,0.418298,-73.334336},{-0.005310,0.078142,-51.688055},{-0.000005,0.002599,-0.821331}};
    //double change_un_Mat[3][3] ={{-0.400000,0.454526,-91.168103},{0.000000,0.084052,-58.836207},{0.000000,0.002802,-0.961207}};
    //double change_un_Mat[3][3] ={{-0.400000,0.454526,-77.532328},{-0.000000,0.084052,-56.314655},{-0.000000,0.002802,-0.877155}};
    double (*change_un_Mat)[3] = camera_ipm_mat;
    for (int i = 0; i < RESULT_COL; i++)
    {
        for (int j = 0; j < RESULT_ROW; j++)
        {
            double map_x, map_y;
            transform_ipm_output(i, j, RESULT_COL, RESULT_ROW, 1.0, map_x, map_y);
            double den = change_un_Mat[2][0] * map_x + change_un_Mat[2][1] * map_y + change_un_Mat[2][2];
            int local_x = (int)((change_un_Mat[0][0] * map_x + change_un_Mat[0][1] * map_y + change_un_Mat[0][2]) / den);
            int local_y = (int)((change_un_Mat[1][0] * map_x + change_un_Mat[1][1] * map_y + change_un_Mat[1][2]) / den);
            if (local_x >= 0 && local_y >= 0 && local_y < USED_ROW && local_x < USED_COL)
            {
                PerImg_ip[j][i] = PerImg_ip_1[local_y][local_x];
            }
            else
            {
                PerImg_ip[j][i] = &BlackColor;
            }
        }
    }
}

uint8_t *PerImg_Color_ip_1[RESULT_ROW][RESULT_COL];
uint8_t *PerImg_Color_ip[RESULT_ROW][RESULT_COL];
uint8_t *PerImg_Color_ip_big[RESULT_ROW * 2][RESULT_COL * 2];
uint8_t BlackColorRGB[3] = {0, 0, 0}; // 越界时的黑色填充

void ImagePerspective1_Color_Init()
{
    double cameraMatrix[3][3] = {{92.055875, 0.000000, 166.461189}, {0.000000, 92.320304, 115.837917}, {0.000000, 0.000000, 1.000000}};
    double distCoeffs[5] = {-0.014048, -0.013327, -0.001824, -0.001394, 0.002692};
    int move_xy[2] = {0, 0};

    double fx = cameraMatrix[0][0], fy = cameraMatrix[1][1], ux = cameraMatrix[0][2], uy = cameraMatrix[1][2];
    double k1 = distCoeffs[0], k2 = distCoeffs[1], k3 = distCoeffs[4], p1 = distCoeffs[2], p2 = distCoeffs[3];
    int move_x = move_xy[0], move_y = move_xy[1];

    for (int i = -move_y; i < RESULT_ROW - move_y; i++)
    {
        for (int j = -move_x; j < RESULT_COL - move_x; j++)
        {
            double xC = (j - ux) / fx;
            double yC = (i - uy) / fy;
            double r2 = xC * xC + yC * yC;
            double deltaRa = 1. + k1 * r2 + k2 * r2 * r2 + k3 * r2 * r2 * r2;
            double deltaTx = 2. * p1 * xC * yC + p2 * (r2 + 2. * xC * xC);
            double deltaTy = p1 * (r2 + 2. * yC * yC) + 2. * p2 * xC * yC;
            double xDist = (xC * deltaRa + deltaTx) * fx + ux;
            double yDist = (yC * deltaRa + deltaTy) * fy + uy;

            // 合并 flip(frame, -1) 的效果：XY 坐标镜像翻转
            //double xDistF = USED_COL - 1 - xDist;
            //double yDistF = USED_ROW - 1 - yDist;
            double xDistF = xDist;
            double yDistF = yDist;

            if (i + move_y >= 0 && i + move_y < RESULT_ROW && j + move_x >= 0 && j + move_x < RESULT_COL)
            {
                if (yDistF >= 0 && yDistF < USED_ROW && xDistF >= 0 && xDistF < USED_COL)
                {
                    int xi = (int)xDistF;
                    int yi = (int)yDistF;
                    int macro_x = xi / 2;
                    int is_odd  = xi % 2;
                    // YUYV: 每行 USED_COL*2 字节，每个宏像素 [Y0,U,Y1,V] 占 4 字节
                    uint8_t *macro_ptr = &p[yi * USED_COL * 2 + macro_x * 4];
                    // 最低位编码奇偶 (0=Y0 偶像素, 1=Y1 奇像素)
                    PerImg_Color_ip_1[i + move_y][j + move_x] = (uint8_t*)((uintptr_t)macro_ptr | is_odd);
                }
                else
                {
                    PerImg_Color_ip_1[i + move_y][j + move_x] = nullptr;
                }
            }
        }
    }
}

void ImagePerspective2_Color_Init()
{
   //double M[3][3] ={{-0.368950,0.418298,-73.334336},{-0.005310,0.078142,-51.688055},{-0.000005,0.002599,-0.821331}};
   //double M[3][3] ={{-0.400000,0.454526,-91.168103},{0.000000,0.084052,-58.836207},{0.000000,0.002802,-0.961207}};
   //double M[3][3] ={{-0.400000,0.454526,-77.532328},{-0.000000,0.084052,-56.314655},{-0.000000,0.002802,-0.877155}};
   double (*change_un_Mat)[3] = camera_ipm_mat;
   for (int i = 0; i < RESULT_COL; i++)
    {
        for (int j = 0; j < RESULT_ROW; j++)
        {
            double map_x, map_y;
            transform_ipm_output(i, j, RESULT_COL, RESULT_ROW, 1.0, map_x, map_y);
            double den = change_un_Mat[2][0] * map_x + change_un_Mat[2][1] * map_y + change_un_Mat[2][2];
            int local_x = (int)((change_un_Mat[0][0] * map_x + change_un_Mat[0][1] * map_y + change_un_Mat[0][2]) / den);
            int local_y = (int)((change_un_Mat[1][0] * map_x + change_un_Mat[1][1] * map_y + change_un_Mat[1][2]) / den);
            
            if (local_x >= 0 && local_y >= 0 && local_y < USED_ROW && local_x < USED_COL)
            {
                PerImg_Color_ip[j][i] = PerImg_Color_ip_1[local_y][local_x];
            }
            else
            {
                PerImg_Color_ip[j][i] = nullptr;
            }
        }
    }
}

void ImagePerspective3_Color_Init()
{
    /*double M[3][3] = {
        {-0.368950 / 2, 0.418298 / 2, -73.334336},
        {-0.005310 / 2, 0.078142 / 2, -51.688055},
        {-0.000005 / 2, 0.002599 / 2, -0.821331}};*/

    /* double M[3][3] = {
        {-0.400000 / 2, 0.454526 / 2, -91.168103},
        {0.000000 / 2,  0.084052 / 2, -58.836207},
        {0.000000 / 2,  0.002802 / 2, -0.961207}}; */
    
    /*double change_un_Mat[3][3] = {
        {-0.400000 / 2, 0.454526 / 2, -77.532328},
        {-0.000000 / 2, 0.084052 / 2, -56.314655},
        {-0.000000 / 2, 0.002802 / 2, -0.877155}}; */
    double change_un_Mat[3][3] = {
        {camera_ipm_mat[0][0] / 2, camera_ipm_mat[0][1] / 2, camera_ipm_mat[0][2]},
        {camera_ipm_mat[1][0] / 2, camera_ipm_mat[1][1] / 2, camera_ipm_mat[1][2]},
        {camera_ipm_mat[2][0] / 2, camera_ipm_mat[2][1] / 2, camera_ipm_mat[2][2]}};
    for (int i = 0; i < RESULT_COL * 2; i++)
    {
        for (int j = 0; j < RESULT_ROW * 2; j++)
        {
            double map_x, map_y;
            transform_ipm_output(i, j, RESULT_COL * 2, RESULT_ROW * 2, 2.0, map_x, map_y);
            double den = change_un_Mat[2][0] * map_x + change_un_Mat[2][1] * map_y + change_un_Mat[2][2];
            int local_x = (int)((change_un_Mat[0][0] * map_x + change_un_Mat[0][1] * map_y + change_un_Mat[0][2]) / den);
            int local_y = (int)((change_un_Mat[1][0] * map_x + change_un_Mat[1][1] * map_y + change_un_Mat[1][2]) / den);
            
            if (local_x >= 0 && local_y >= 0 && local_y < RESULT_ROW && local_x < RESULT_COL)
            {
                PerImg_Color_ip_big[j][i] = PerImg_Color_ip_1[local_y][local_x];
            }
            else
            {
                PerImg_Color_ip_big[j][i] = nullptr;
            }
        }
    }
}

#define IMG_H 240
#define IMG_W 320
#define BLK 7
#define BLKH (IMG_H / BLK) // 48
#define BLKW (IMG_W / BLK) // 64

int get_image_sum(int y, int x)
{

    if (y < 0 || y >= 240 || x < 0 || x >= 320)
    {

        return 125; // 越界视为125
    }
    return *PerImg_ip[y][x];
}

int get_image_sum_real(int y, int x)
{

    if (y < 0 || y >= 240 || x < 0 || x >= 320)
    {

        return 125; // 越界视为125
    }
    return p[y * cols + x];
    // return *PerImg_ip[y][x];
}

void set_image_sum(int y, int x, int num)
{

    if (y < 0 || y >= 240 || x < 0 || x >= 320)
        return;

    *PerImg_ip[y][x] = num;
}

void set_image_sum_real(int y, int x, int num)
{

    if (y < 0 || y >= 240 || x < 0 || x >= 320)
        return;

    p[y * cols + x] = num;
}

// ==================== 隐藏的 MMAP 内部结构 ====================
struct CamInternal
{
    int fd = -1;
    uint32_t width = 320;
    uint32_t height = 240;
    uint32_t pixel_format = V4L2_PIX_FMT_MJPEG; // 默认改为 MJPEG
    int fps = 120;
    int buffer_count = 2; // 对应 CAP_PROP_BUFFERSIZE

    void *buffer_start[4]; // 最大支持 4 个缓存
    size_t buffer_length[4];
    int n_buffers = 0;
    int current_index = 0;
    tjhandle decompressor = nullptr;
} static g_cam;

static uint32_t exposure_control_id()
{
    struct v4l2_queryctrl query = {0};
    query.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    if (g_cam.fd >= 0 && ioctl(g_cam.fd, VIDIOC_QUERYCTRL, &query) == 0 && !(query.flags & V4L2_CTRL_FLAG_DISABLED))
        return V4L2_CID_EXPOSURE_ABSOLUTE;
    return V4L2_CID_EXPOSURE;
}

static bool get_control(uint32_t id, int &value)
{
    struct v4l2_control ctrl = {0}; ctrl.id = id;
    if (g_cam.fd < 0 || ioctl(g_cam.fd, VIDIOC_G_CTRL, &ctrl) < 0) return false;
    value = ctrl.value; return true;
}

static bool set_control(uint32_t id, int value)
{
    struct v4l2_control ctrl = {0}; ctrl.id = id; ctrl.value = value;
    return g_cam.fd >= 0 && ioctl(g_cam.fd, VIDIOC_S_CTRL, &ctrl) == 0;
}

static bool exposure_mode_supported(int mode)
{
    if (g_cam.fd < 0) return false;
    struct v4l2_querymenu menu = {0};
    menu.id = V4L2_CID_EXPOSURE_AUTO;
    menu.index = mode;
    return ioctl(g_cam.fd, VIDIOC_QUERYMENU, &menu) == 0;
}

static int automatic_exposure_mode()
{
    if (exposure_mode_supported(V4L2_EXPOSURE_AUTO))
        return V4L2_EXPOSURE_AUTO;
    if (exposure_mode_supported(V4L2_EXPOSURE_APERTURE_PRIORITY))
        return V4L2_EXPOSURE_APERTURE_PRIORITY;
    return -1;
}

bool camera_exposure_get(CameraExposureConfig &config)
{
    config = {false, true, exposure, 1, 10000, 1, false, false};
    if (g_cam.fd < 0) return false;
    int mode = V4L2_EXPOSURE_AUTO, priority = 0, value = exposure;
    if (!get_control(V4L2_CID_EXPOSURE_AUTO, mode)) return false;
    const uint32_t exposure_id = exposure_control_id();
    struct v4l2_queryctrl query = {0}; query.id = exposure_id;
    if (ioctl(g_cam.fd, VIDIOC_QUERYCTRL, &query) == 0) {
        config.exposure_min = query.minimum;
        config.exposure_max = query.maximum;
        config.exposure_step = query.step > 0 ? query.step : 1;
    }
    get_control(exposure_id, value);
    config.auto_priority_available = get_control(V4L2_CID_EXPOSURE_AUTO_PRIORITY, priority);
    config.available = true;
    // 本摄像头支持 Aperture Priority(3) 作为自动曝光，不支持 AUTO(0)。
    config.auto_exposure = mode != V4L2_EXPOSURE_MANUAL;
    config.exposure_time = value;
    config.auto_priority = priority != 0;
    return true;
}

bool camera_exposure_save(const CameraExposureConfig &config, std::string &message)
{
    mkdir("/home/root/configs", 0777);
    std::lock_guard<std::mutex> cfg_lock(g_camera_cfg_mutex);
    json j = json::object();
    try {
        std::ifstream existing(CAMERA_CFG_PATH);
        if (existing.is_open()) existing >> j;
        if (!j.is_object()) j = json::object();
    } catch (...) { j = json::object(); }
    j["auto_exposure"] = config.auto_exposure;
    j["exposure"] = config.exposure_time;
    j["exposure_auto_priority"] = config.auto_priority;
    const std::string temp_path = std::string(CAMERA_CFG_PATH) + ".tmp";
    std::ofstream out(temp_path);
    if (!out.is_open()) { message = "无法写入摄像头配置"; return false; }
    out << j.dump(4); out.close();
    if (rename(temp_path.c_str(), CAMERA_CFG_PATH) != 0) {
        unlink(temp_path.c_str()); message = "无法替换摄像头配置"; return false;
    }
    message = "参数已应用并保存";
    return true;
}

bool camera_exposure_set(bool auto_mode, int exposure_time, bool auto_priority,
                         CameraExposureConfig &applied, std::string &message)
{
    CameraExposureConfig current;
    if (!camera_exposure_get(current)) { message = "摄像头曝光控制不可用"; return false; }
    exposure_time = std::max(current.exposure_min, std::min(current.exposure_max, exposure_time));
    int requested_mode = auto_mode ? automatic_exposure_mode() : V4L2_EXPOSURE_MANUAL;
    if (requested_mode < 0 ||
        !set_control(V4L2_CID_EXPOSURE_AUTO, requested_mode)) {
        message = "自动/手动曝光设置失败"; return false;
    }
    if (current.auto_priority_available &&
        !set_control(V4L2_CID_EXPOSURE_AUTO_PRIORITY, auto_priority ? 1 : 0)) {
        message = "自动曝光优先模式设置失败"; return false;
    }
    if (!auto_mode && !set_control(exposure_control_id(), exposure_time)) {
        message = "手动曝光时间设置失败"; return false;
    }
    if (!camera_exposure_get(applied)) { message = "参数已设置，但读取生效值失败"; return false; }
    exposure = applied.exposure_time;
    auto_exposure = applied.auto_exposure ? 1 : 0;
    bool saved = camera_exposure_save(applied, message);
    if (saved && !applied.auto_priority_available)
        message = "曝光参数已应用并保存；当前摄像头不支持帧率/亮度优先设置";
    return saved;
}

static bool image_control_get(uint32_t id, CameraControlValue &item)
{
    item = {false, false, 0, 0, 0, 1};
    if (g_cam.fd < 0) return false;
    struct v4l2_queryctrl query = {0}; query.id = id;
    if (ioctl(g_cam.fd, VIDIOC_QUERYCTRL, &query) < 0 ||
        (query.flags & (V4L2_CTRL_FLAG_DISABLED | V4L2_CTRL_FLAG_READ_ONLY |
                        V4L2_CTRL_FLAG_WRITE_ONLY))) return false;
    int value = 0;
    if (!get_control(id, value)) return false;
    item.available = true;
    item.inactive = (query.flags & V4L2_CTRL_FLAG_INACTIVE) != 0;
    item.value = value;
    item.minimum = query.minimum;
    item.maximum = query.maximum;
    item.step = query.step > 0 ? query.step : 1;
    return true;
}

bool camera_image_controls_get(CameraImageControls &controls)
{
    bool any = false;
    any |= image_control_get(V4L2_CID_BRIGHTNESS, controls.brightness);
    any |= image_control_get(V4L2_CID_CONTRAST, controls.contrast);
    any |= image_control_get(V4L2_CID_SATURATION, controls.saturation);
    any |= image_control_get(V4L2_CID_HUE, controls.hue);
    any |= image_control_get(V4L2_CID_AUTO_WHITE_BALANCE, controls.auto_white_balance);
    any |= image_control_get(V4L2_CID_GAMMA, controls.gamma);
    any |= image_control_get(V4L2_CID_POWER_LINE_FREQUENCY, controls.power_line_frequency);
    any |= image_control_get(V4L2_CID_WHITE_BALANCE_TEMPERATURE, controls.white_balance_temperature);
    any |= image_control_get(V4L2_CID_SHARPNESS, controls.sharpness);
    return any;
}

static bool valid_image_control_value(const CameraControlValue &control, int value)
{
    return control.available && value >= control.minimum && value <= control.maximum &&
           ((value - control.minimum) % control.step == 0);
}

static bool apply_image_control(uint32_t id, const CameraControlValue &current,
                                const CameraControlValue &requested, std::string &message)
{
    if (!current.available) return true;
    if (current.inactive) return true;
    if (!valid_image_control_value(current, requested.value)) {
        message = "参数值超出摄像头允许范围"; return false;
    }
    if (!set_control(id, requested.value)) {
        message = "摄像头拒绝设置画面参数"; return false;
    }
    return true;
}

static bool save_image_controls(const CameraImageControls &controls, std::string &message)
{
    mkdir("/home/root/configs", 0777);
    std::lock_guard<std::mutex> cfg_lock(g_camera_cfg_mutex);
    json j = json::object();
    try {
        std::ifstream existing(CAMERA_CFG_PATH);
        if (existing.is_open()) existing >> j;
        if (!j.is_object()) j = json::object();
    } catch (...) { j = json::object(); }
    json values = json::object();
    values["brightness"] = controls.brightness.value;
    values["contrast"] = controls.contrast.value;
    values["saturation"] = controls.saturation.value;
    values["hue"] = controls.hue.value;
    values["auto_white_balance"] = controls.auto_white_balance.value;
    values["gamma"] = controls.gamma.value;
    values["power_line_frequency"] = controls.power_line_frequency.value;
    values["white_balance_temperature"] = controls.white_balance_temperature.value;
    values["sharpness"] = controls.sharpness.value;
    j["image_controls"] = values;
    const std::string temp_path = std::string(CAMERA_CFG_PATH) + ".tmp";
    std::ofstream out(temp_path);
    if (!out.is_open()) { message = "无法写入摄像头配置"; return false; }
    out << j.dump(4); out.close();
    if (rename(temp_path.c_str(), CAMERA_CFG_PATH) != 0) {
        unlink(temp_path.c_str()); message = "无法替换摄像头配置"; return false;
    }
    return true;
}

bool camera_image_controls_set(const CameraImageControls &requested,
                               CameraImageControls &applied, std::string &message)
{
    CameraImageControls current;
    if (!camera_image_controls_get(current)) { message = "摄像头画面参数不可用"; return false; }
    // 自动白平衡决定色温控件是否可用，必须先设置并重新查询状态。
    if (!apply_image_control(V4L2_CID_AUTO_WHITE_BALANCE, current.auto_white_balance,
                             requested.auto_white_balance, message)) return false;
    camera_image_controls_get(current);
    if (!apply_image_control(V4L2_CID_BRIGHTNESS, current.brightness, requested.brightness, message) ||
        !apply_image_control(V4L2_CID_CONTRAST, current.contrast, requested.contrast, message) ||
        !apply_image_control(V4L2_CID_SATURATION, current.saturation, requested.saturation, message) ||
        !apply_image_control(V4L2_CID_HUE, current.hue, requested.hue, message) ||
        !apply_image_control(V4L2_CID_GAMMA, current.gamma, requested.gamma, message) ||
        !apply_image_control(V4L2_CID_POWER_LINE_FREQUENCY, current.power_line_frequency,
                             requested.power_line_frequency, message) ||
        !apply_image_control(V4L2_CID_WHITE_BALANCE_TEMPERATURE, current.white_balance_temperature,
                             requested.white_balance_temperature, message) ||
        !apply_image_control(V4L2_CID_SHARPNESS, current.sharpness, requested.sharpness, message)) return false;
    if (!camera_image_controls_get(applied)) { message = "参数已设置，但读取生效值失败"; return false; }
    if (!save_image_controls(applied, message)) return false;
    message = "画面参数已应用并保存";
    return true;
}

void camera_image_controls_load_saved()
{
    if (g_cam.fd < 0) return;
    try {
        std::ifstream input(CAMERA_CFG_PATH);
        json j;
        if (!input.is_open()) return;
        input >> j;
        if (!j.contains("image_controls") || !j["image_controls"].is_object()) return;
        CameraImageControls requested;
        if (!camera_image_controls_get(requested)) return;
        const json &v = j["image_controls"];
        requested.brightness.value = v.value("brightness", requested.brightness.value);
        requested.contrast.value = v.value("contrast", requested.contrast.value);
        requested.saturation.value = v.value("saturation", requested.saturation.value);
        requested.hue.value = v.value("hue", requested.hue.value);
        requested.auto_white_balance.value = v.value("auto_white_balance", requested.auto_white_balance.value);
        requested.gamma.value = v.value("gamma", requested.gamma.value);
        requested.power_line_frequency.value = v.value("power_line_frequency", requested.power_line_frequency.value);
        requested.white_balance_temperature.value = v.value("white_balance_temperature", requested.white_balance_temperature.value);
        requested.sharpness.value = v.value("sharpness", requested.sharpness.value);
        CameraImageControls applied; std::string message;
        camera_image_controls_set(requested, applied, message);
    } catch (...) {}
}

// ==================== 底层工具函数 (仅限本文件使用) ====================

static int v4l2_init_device()
{
    std::string dev_path = find_camera_device();
    if (dev_path.empty())
    {
        cerr << "Error: No usable USB camera found for MMAP!" << endl;
        return -1;
    }

    // 阻塞模式打开自动找到的设备
    g_cam.fd = open(dev_path.c_str(), O_RDWR);
    if (g_cam.fd < 0)
    {
        perror("Opening video device");
        return -1;
    }

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = g_cam.width;
    fmt.fmt.pix.height = g_cam.height;
    fmt.fmt.pix.pixelformat = g_cam.pixel_format;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(g_cam.fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("Setting Pixel Format");
        return -1;
    }

    // 同步实际生效的宽高
    g_cam.width = fmt.fmt.pix.width;
    g_cam.height = fmt.fmt.pix.height;
    g_cam.pixel_format = fmt.fmt.pix.pixelformat;

    // 设置帧率
    struct v4l2_streamparm streamparm = {0};
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamparm.parm.capture.timeperframe.numerator = 1;
    streamparm.parm.capture.timeperframe.denominator = g_cam.fps;
    ioctl(g_cam.fd, VIDIOC_S_PARM, &streamparm);

    // 映射缓冲区
    struct v4l2_requestbuffers req = {0};
    req.count = g_cam.buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(g_cam.fd, VIDIOC_REQBUFS, &req) < 0)
        return -1;

    for (int i = 0; i < (int)req.count; ++i)
    {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ioctl(g_cam.fd, VIDIOC_QUERYBUF, &buf);

        g_cam.buffer_length[i] = buf.length;
        g_cam.buffer_start[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, g_cam.fd, buf.m.offset);
        ioctl(g_cam.fd, VIDIOC_QBUF, &buf);
    }
    g_cam.n_buffers = req.count;
    cout << "MMAP initialized on: " << dev_path << endl;
    return 0;
}

// ==================== 暴露给用户的三个 API ====================

void camera_mmap_init()
{
    g_cam.width = 320;
    g_cam.height = 240;
    g_cam.fps = 120;
    g_cam.buffer_count = 2;
    g_cam.pixel_format = V4L2_PIX_FMT_YUYV; // 你可以根据需要改为 MJPEG

    if (v4l2_init_device() < 0)
        return;

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(g_cam.fd, VIDIOC_STREAMON, &type);

    if (g_cam.pixel_format == V4L2_PIX_FMT_MJPEG)
    {
        g_cam.decompressor = tjInitDecompress();
    }

    frame = Mat::zeros(g_cam.height, g_cam.width, CV_8UC3);

    // 预分配固定缓冲区，保证 p 地址不变，避免透视表悬空指针
    g_yuyv.create(g_cam.height, g_cam.width, CV_8UC2);

    // 初始化透视表
    camera_cfg_load();
    try {
        std::ifstream cfg_file(CAMERA_CFG_PATH);
        json cfg_json; if (cfg_file.is_open()) cfg_file >> cfg_json;
        CameraExposureConfig applied; std::string message;
        camera_exposure_set(cfg_json.value("auto_exposure", true),
                            cfg_json.value("exposure", exposure),
                            cfg_json.value("exposure_auto_priority", false), applied, message);
    } catch (...) {}
    camera_image_controls_load_saved();
    get_mmap_image(1);
    ImagePerspective1_Init();
    ImagePerspective2_Init();
    get_mmap_image(3);
    ImagePerspective1_Color_Init();
    ImagePerspective2_Color_Init();
    ImagePerspective3_Color_Init();
    cout << "MMAP Camera Started Successfully." << endl;
}

int get_mmap_image(int setting)
{
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(g_cam.fd, VIDIOC_DQBUF, &buf) < 0)
        return -1;

    uint8_t *raw_ptr = (uint8_t *)g_cam.buffer_start[buf.index];
    // 使用固定缓冲区：memcpy 替代 clone()，保证 p 地址不变，避免透视表悬空指针
    memcpy(g_yuyv.data, raw_ptr, g_cam.height * g_cam.width * 2);

    // --- 灰度 / 二值化模式 ---
    // setting 1/2：直接使用 Y 分量；setting 4/5：使用白色得分
    // Y - (|U - 128| + |V - 128|) / 2。
    if (setting == 1 || setting == 2 || setting == 4 || setting == 5)
    {
        if (setting == 1 || setting == 2)
        {
            extractChannel(g_yuyv, grayImage, 0);
        }
        else
        {
            grayImage.create(g_cam.height, g_cam.width, CV_8UC1);
            const int height = static_cast<int>(g_cam.height);
            const int width = static_cast<int>(g_cam.width);
            for (int y = 0; y < height; ++y)
            {
                const uint8_t *src = g_yuyv.ptr<uint8_t>(y);
                uint8_t *dst = grayImage.ptr<uint8_t>(y);
                for (int x = 0; x + 1 < width; x += 2)
                {
                    // YUYV 宏像素：[Y0, U, Y1, V]，两个 Y 像素共享 U/V。
                    const int chroma_penalty =
                        (std::abs((int)src[1] - 128) + std::abs((int)src[3] - 128)) >> 1;
                    const int num0 = (int)src[0] - chroma_penalty;
                    const int num1 = (int)src[2] - chroma_penalty;
                    dst[x] = (uint8_t)(num0 < 0 ? 0 : (num0 > 255 ? 255 : num0));
                    dst[x + 1] = (uint8_t)(num1 < 0 ? 0 : (num1 > 255 ? 255 : num1));
                    src += 4;
                }
            }
        }

        if (setting == 1 || setting == 4) // 二值化模式
        {
            float threshold_num = getStandardOtsuThreshold(grayImage, image_scale);
            threshold(grayImage, binaryImage, threshold_num, 255, THRESH_BINARY);
            p = binaryImage.data;
            rows = binaryImage.rows;
            cols = binaryImage.cols;
        }
        else // 灰度模式
        {
            p = grayImage.data;
            rows = grayImage.rows;
            cols = grayImage.cols;
        }
    }
    // --- 彩色模式：保存 YUYV 原图，不做全图 BGR 转换 ---
    else if (setting == 3)
    {   
        p = g_yuyv.data;
        rows = g_yuyv.rows;
        cols = g_yuyv.cols;          // 像素列数（每行字节数 = cols * 2）
    }

    ioctl(g_cam.fd, VIDIOC_QBUF, &buf);
    return 0;
}

void camera_mmap_clean()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (g_cam.fd != -1)
        ioctl(g_cam.fd, VIDIOC_STREAMOFF, &type);

    for (int i = 0; i < g_cam.n_buffers; ++i)
    {
        if (g_cam.buffer_start[i] != MAP_FAILED)
            munmap(g_cam.buffer_start[i], g_cam.buffer_length[i]);
    }

    if (g_cam.decompressor)
        tjDestroy(g_cam.decompressor);
    if (g_cam.fd != -1)
        close(g_cam.fd);
    g_cam.fd = -1;
}

// 从 PerImg_Color_ip 截取正方形区域，保存到全局 Mat square（BGR 格式）
// (y_center, x_center): 正方形中心坐标
// length: 边长
void cut_image(int y_center, int x_center, int length) {
    int half = length / 2;
    int y_start = y_center - half;
    int x_start = x_center - half;
    const int8_t (*dy_map)[75];
    const int8_t (*dx_map)[75];

    // angle_barrier_save：相对屏幕竖直向上的夹角，顺时针为正。
    // 反向采样表在图像坐标中同样以顺时针为正，因此正角选择 p 表、
    // 负角选择 n 表。阈值位于相邻映射角的中点，残余角不超过 11.25 度。
    if (angle_barrier_save < -78.75f) {
        dy_map = rotate_75_75_n90_y;
        dx_map = rotate_75_75_n90_x;
    } else if (angle_barrier_save < -56.25f) {
        dy_map = rotate_75_75_n67_5_y;
        dx_map = rotate_75_75_n67_5_x;
    } else if (angle_barrier_save < -33.75f) {
        dy_map = rotate_75_75_n45_y;
        dx_map = rotate_75_75_n45_x;
    } else if (angle_barrier_save < -11.25f) {
        dy_map = rotate_75_75_n22_5_y;
        dx_map = rotate_75_75_n22_5_x;
    } else if (angle_barrier_save <= 11.25f) {
        dy_map = rotate_75_75_0_y;
        dx_map = rotate_75_75_0_x;
    } else if (angle_barrier_save <= 33.75f) {
        dy_map = rotate_75_75_p22_5_y;
        dx_map = rotate_75_75_p22_5_x;
    } else if (angle_barrier_save <= 56.25f) {
        dy_map = rotate_75_75_p45_y;
        dx_map = rotate_75_75_p45_x;
    } else if (angle_barrier_save <= 78.75f) {
        dy_map = rotate_75_75_p67_5_y;
        dx_map = rotate_75_75_p67_5_x;
    } else {
        dy_map = rotate_75_75_p90_y;
        dx_map = rotate_75_75_p90_x;
    }

    square.create(length, length, CV_8UC3);

    for (int i = 0; i < length; i++) {
        
        Vec3b *row_ptr = square.ptr<Vec3b>(i);
        for (int j = 0; j < length; j++) {
            int src_y = y_start + i + dy_map[i][j];
            int src_x = x_start + j + dx_map[i][j];
            if (src_y >= 0 && src_y < RESULT_ROW * 2 && src_x >= 0 && src_x < RESULT_COL * 2) {
                uint8_t *ptr = PerImg_Color_ip_big[src_y][src_x];
                if (ptr != nullptr) {
                    // 解码指针最低位的奇偶标记
                    bool is_odd = ((uintptr_t)ptr & 1) != 0;
                    uint8_t *base = (uint8_t *)((uintptr_t)ptr & ~(uintptr_t)1);

                    // YUYV 宏像素: [Y0, U, Y1, V]
                    int Y = is_odd ? base[2] : base[0];
                    int U = base[1];
                    int V = base[3];

                    // BT.601 YUV (limited range) -> BGR
                    int C = Y - 16;
                    int D = U - 128;
                    int E = V - 128;

                    int r = (298 * C + 409 * E + 128) >> 8;
                    int g = (298 * C - 100 * D - 208 * E + 128) >> 8;
                    int b = (298 * C + 516 * D + 128) >> 8;

                    r = r < 0 ? 0 : (r > 255 ? 255 : r);
                    g = g < 0 ? 0 : (g > 255 ? 255 : g);
                    b = b < 0 ? 0 : (b > 255 ? 255 : b);

                    row_ptr[j] = Vec3b((uchar)b, (uchar)g, (uchar)r);
                } else {
                    row_ptr[j] = Vec3b(178, 178, 178);
                }
            } else {
                row_ptr[j] = Vec3b(178, 178, 178);
            }
        }
    }
}

/**
 * @brief 测试 MMAP 模式下获取 500 帧的 CPU 占用率
 * 此函数会模拟实际运行环境，获取图像并进行二值化处理
 */
void camera_mmap_perf_test()
{
    const int test_frames = 500;
    std::cout << "\n--- Starting Performance Test (" << test_frames << " frames) ---" << std::endl;

    // 确保摄像头已初始化
    if (g_cam.fd == -1)
    {
        camera_mmap_init();
    }
    cout << "Start with 320*240@120fps" << endl;
    // 记录开始时间
    uint64_t start_wall = get_wall_time_ns();
    uint64_t start_cpu = get_cpu_time_ns();

    for (int i = 0; i < test_frames; ++i)
    {
        get_mmap_image(1);
    }

    // 记录结束时间
    uint64_t end_wall = get_wall_time_ns();
    uint64_t end_cpu = get_cpu_time_ns();

    // 计算差值
    double wall_time_ms = (end_wall - start_wall) / 1000000.0;
    double cpu_time_ms = (end_cpu - start_cpu) / 1000000.0;

    // 计算占用率 (CPU时间 / 实际经过时间)
    double cpu_usage = (cpu_time_ms / wall_time_ms) * 100.0;
    double avg_fps = test_frames / (wall_time_ms / 1000.0);

    // 打印结果
    std::cout << "-----------------------------------------------" << std::endl;
    std::cout << "Test Results:" << std::endl;
    std::cout << "Total Wall Time: " << std::fixed << std::setprecision(2) << wall_time_ms << " ms" << std::endl;
    std::cout << "Total CPU Time:  " << cpu_time_ms << " ms" << std::endl;
    std::cout << "Average FPS:     " << avg_fps << " fps" << std::endl;
    std::cout << "CPU Usage Rate:  " << cpu_usage << " %" << std::endl;
    std::cout << "-----------------------------------------------\n"
              << std::endl;
}

void get_camera_mmap_message()
{
    if (g_cam.fd < 0)
    {
        cerr << "Error: MMAP camera not initialized!" << endl;
        return;
    }

    // 查询当前格式（宽高、像素格式）
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(g_cam.fd, VIDIOC_G_FMT, &fmt) < 0)
    {
        cerr << "Error: Failed to get format!" << endl;
        return;
    }
    double frameWidth = fmt.fmt.pix.width;
    double frameHeight = fmt.fmt.pix.height;
    unsigned int pixelformat = fmt.fmt.pix.pixelformat;

    // 查询帧率
    struct v4l2_streamparm parm = {0};
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    double fps = 0;
    if (ioctl(g_cam.fd, VIDIOC_G_PARM, &parm) >= 0)
    {
        fps = (double)parm.parm.capture.timeperframe.denominator
            / parm.parm.capture.timeperframe.numerator;
    }

    double buffersize = g_cam.buffer_count;

    // 使用统一接口读取实际曝光时间和曝光模式。
    // 曝光时间优先查询 V4L2_CID_EXPOSURE_ABSOLUTE，并兼容 V4L2_CID_EXPOSURE。
    CameraExposureConfig exposure_config;
    const bool exposure_available = camera_exposure_get(exposure_config);

    // 输出格式与 get_camera_message() 保持一致
    std::cout << "Camera Parameters (MMAP):" << std::endl;
    std::cout << "Frame Width:   " << frameWidth << std::endl;
    std::cout << "Frame Height:  " << frameHeight << std::endl;
    std::cout << "FPS:           " << fps << std::endl;
    std::cout << "FOURCC:        " << (char)(pixelformat & 0xFF)
              << (char)((pixelformat >> 8) & 0xFF)
              << (char)((pixelformat >> 16) & 0xFF)
              << (char)((pixelformat >> 24) & 0xFF)
              << std::endl;
    std::cout << "Buffer Size:   " << buffersize << std::endl;
    if (exposure_available)
    {
        std::cout << "Exposure:      " << exposure_config.exposure_time << std::endl;
        std::cout << "Auto Exposure: "
                  << (exposure_config.auto_exposure ? "Automatic" : "Manual")
                  << std::endl;
    }
    else
    {
        std::cout << "Exposure:      Unavailable" << std::endl;
        std::cout << "Auto Exposure: Unavailable" << std::endl;
    }
}

// ==================== YUYV → HSV 转换 ====================

/**
 * @brief 将 PerImg_Color_ip[y][x] 的 YUYV 像素转换为 HSV
 * @param y 行坐标 (0 ~ RESULT_ROW-1)
 * @param x 列坐标 (0 ~ RESULT_COL-1)
 * @return HSV 结构体，越界或空指针时返回全零
 */
HSV yuyv_to_hsv(int y, int x) {
    HSV result = {0.0f, 0.0f, 0.0f};

    // 边界检查
    if (y < 0 || y >= RESULT_ROW || x < 0 || x >= RESULT_COL) {
        return result;
    }

    uint8_t *ptr = PerImg_Color_ip[y][x];
    if (ptr == nullptr) {
        return result;
    }

    // --- 第一步：解码 YUYV 宏像素 ---
    bool is_odd = ((uintptr_t)ptr & 1) != 0;
    uint8_t *base = (uint8_t *)((uintptr_t)ptr & ~(uintptr_t)1);

    int Y = is_odd ? base[2] : base[0];
    int U = base[1];
    int V = base[3];

    // --- 第二步：BT.601 YUV (limited range) → RGB ---
    int C = Y - 16;
    int D = U - 128;
    int E = V - 128;

    int r = (298 * C + 409 * E + 128) >> 8;
    int g = (298 * C - 100 * D - 208 * E + 128) >> 8;
    int b = (298 * C + 516 * D + 128) >> 8;

    // clamp to 0~255
    r = r < 0 ? 0 : (r > 255 ? 255 : r);
    g = g < 0 ? 0 : (g > 255 ? 255 : g);
    b = b < 0 ? 0 : (b > 255 ? 255 : b);

    // --- 第三步：RGB → HSV ---
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float max_val = rf;
    if (gf > max_val) max_val = gf;
    if (bf > max_val) max_val = bf;

    float min_val = rf;
    if (gf < min_val) min_val = gf;
    if (bf < min_val) min_val = bf;

    float delta = max_val - min_val;

    result.v = max_val;

    if (max_val == 0.0f) {
        // 纯黑
        result.s = 0.0f;
        result.h = 0.0f;
    } else {
        result.s = delta / max_val;

        if (delta == 0.0f) {
            // 灰色（无色相）
            result.h = 0.0f;
        } else {
            if (max_val == rf) {
                result.h = 60.0f * ((gf - bf) / delta);
            } else if (max_val == gf) {
                result.h = 60.0f * (2.0f + (bf - rf) / delta);
            } else {
                result.h = 60.0f * (4.0f + (rf - gf) / delta);
            }

            if (result.h < 0.0f) {
                result.h += 360.0f;
            }
        }
    }

    return result;
}
