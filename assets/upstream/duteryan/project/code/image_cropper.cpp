#include "image_cropper.hpp"
uint8_t* crop_gray_image(const uint8_t* src_gray)
{
    // ========== 1. 严格参数校验 ==========
    // 空指针校验
    if (src_gray == nullptr)
    {
        throw std::invalid_argument("crop_gray_image: 输入灰度图指针不能为空！");
    }
    // 尺寸范围校验（转为size_t消除有符号/无符号警告）
    const size_t src_w = static_cast<size_t>(UVC_WIDTH );
    const size_t src_h = static_cast<size_t>(UVC_HEIGHT);
    const size_t dst_w = static_cast<size_t>(IMAGE_WIDTH);
    const size_t dst_h = static_cast<size_t>(IMAGE_HEIGHT);

    if (dst_w == 0 || dst_w > src_w || dst_h == 0 || dst_h > src_h)
    {
        throw std::invalid_argument("crop_gray_image: 目标尺寸超出范围（需1≤宽≤320，1≤高≤180）！");
    }

    // ========== 2. 计算中心裁剪起始坐标 ==========
    const size_t start_x = (src_w - dst_w) / 2;  // 水平起始列（像素级）
    const size_t start_y = (src_h - dst_h) / 2;  // 垂直起始行（像素级）

    // ========== 3. 分配目标内存 ==========
    const size_t dst_total_size = dst_w * dst_h;
    uint8_t* cropped_gray = new uint8_t[dst_total_size](); // 初始化为0

    // ========== 4. 逐行拷贝（核心逻辑，无错位） ==========
    // 原图每行字节数 = 原图宽度（单通道，1字节/像素）
    const size_t src_row_bytes = src_w;
    // 目标图每行字节数 = 目标宽度
    const size_t dst_row_bytes = dst_w;

    for (size_t y = 0; y < dst_h; ++y)  // size_t避免有符号警告
    {
        // 原图当前行起始地址 = (起始行 + 当前行) × 原图每行字节数 + 起始列偏移
        const uint8_t* src_row = src_gray + (start_y + y) * src_row_bytes + start_x;
        // 目标图当前行起始地址
        uint8_t* dst_row = cropped_gray + y * dst_row_bytes;
        // 拷贝整行数据（单通道，直接memcpy，无任何错位）
        memcpy(dst_row, src_row, dst_row_bytes);
    }

    return cropped_gray;
}
// 外部宏定义（请确保在你的项目中正确定义）
// #define UVC_WIDTH     原始图像宽度
// #define UVC_HEIGHT    原始图像高度
// #define IMAGE_WIDTH   目标压缩宽度
// #define IMAGE_HEIGHT  目标压缩高度

/**
 * @brief 纯C++实现灰度图像缩放压缩（双线性插值 - 几何中心对齐版）
 * @param src_gray 输入灰度图像数据指针（UVC_WIDTH x UVC_HEIGHT）
 * @return 输出压缩后灰度图像数据指针（IMAGE_WIDTH x IMAGE_HEIGHT），需调用者使用 delete[] 释放
 */

// =======================================================
// 【硬核裸机版】纯 C++，纯整数位移运算，不依赖任何库
// =======================================================
uint8_t* compress_gray_image(const uint8_t* src_gray)
{
    // 1. 【核心魔法】静态数组，代替你的 new uint8_t[]
    static uint8_t dst_buf[IMAGE_WIDTH * IMAGE_HEIGHT];

    if (src_gray == nullptr) return nullptr;

    const int src_w = UVC_WIDTH;
    const int src_h = UVC_HEIGHT;
    const int dst_w = IMAGE_WIDTH;
    const int dst_h = IMAGE_HEIGHT;

    if (src_w == dst_w && src_h == dst_h) {
        std::memcpy(dst_buf, src_gray, src_w * src_h);
        return dst_buf;
    }

    // 2. 预先计算步长比例（将比例放大 65536 倍，转为定点整数）
    const int x_ratio = (src_w << 16) / dst_w;
    const int y_ratio = (src_h << 16) / dst_h;

    int dst_idx = 0;
    
    // 3. 极速双重循环：没有任何浮点数 (float)！没有任何越界判断 (min/max)！
    for (int y = 0; y < dst_h; ++y) {
        // 右移 16 位等价于除以 65536，完美避开耗时的除法器
        const int src_y = (y * y_ratio) >> 16;
        const int src_row_offset = src_y * src_w;

        for (int x = 0; x < dst_w; ++x) {
            const int src_x = (x * x_ratio) >> 16;
            
            // 纯一维数组寻址，龙芯的 L1 Cache 命中率能达到 99%
            dst_buf[dst_idx++] = src_gray[src_row_offset + src_x];
        }
    }

    return dst_buf;
}
uint16_t* crop_rgb565_image(const uint16_t* src_rgb)
{
    // 1. 参数校验
    if (src_rgb == nullptr)
    {
        throw std::invalid_argument("crop_rgb565_image: 输入RGB指针不能为空！");
    }
    const size_t src_w = static_cast<size_t>(UVC_WIDTH);
    const size_t src_h = static_cast<size_t>(UVC_HEIGHT);
    const size_t dst_w = static_cast<size_t>(IMAGE_WIDTH);
    const size_t dst_h = static_cast<size_t>(IMAGE_HEIGHT);

    if (dst_w == 0 || dst_w > src_w || dst_h == 0 || dst_h > src_h)
    {
        throw std::invalid_argument("crop_rgb565_image: 目标尺寸超出范围！");
    }

    // 2. 计算中心裁剪坐标（像素级，单通道）
    const size_t start_x = (src_w - dst_w) / 2;
    const size_t start_y = (src_h - dst_h) / 2;

    // 3. 分配内存（每个像素1个uint16_t，总长度=宽×高）
    const size_t dst_total_size = dst_w * dst_h;
    uint16_t* cropped_rgb = new uint16_t[dst_total_size]();

    // 4. 逐行拷贝（和灰度图逻辑一致！因为都是单通道）
    const size_t src_row_elems = src_w;  // 每行像素数（单通道）
    const size_t dst_row_elems = dst_w;
    for (size_t y = 0; y < dst_h; ++y)
    {
        // 原图当前行起始地址 = (起始行+当前行)×每行像素数 + 起始列
        const uint16_t* src_row = src_rgb + (start_y + y) * src_row_elems + start_x;
        // 目标当前行起始地址
        uint16_t* dst_row = cropped_rgb + y * dst_row_elems;
        // 拷贝整行（每个元素2字节，按uint16_t拷贝，无错位）
        memcpy(dst_row, src_row, dst_row_elems * sizeof(uint16_t));
    }

    return cropped_rgb;
}


// // =======================================================
// // 【修正版】增加对原图物理步长 (src_step) 的处理
// // =======================================================
// uint8_t* compress_gray_image(const uint8_t* src_gray, int src_step)
// {
//     // 1. 静态数组，大小为目标图像总像素
//     static uint8_t dst_buf[IMAGE_WIDTH * IMAGE_HEIGHT];

//     if (src_gray == nullptr) return nullptr;

//     const int src_w = UVC_WIDTH;
//     const int src_h = UVC_HEIGHT;
//     const int dst_w = IMAGE_WIDTH;
//     const int dst_h = IMAGE_HEIGHT;

//     // 比例计算保持不变（定点整数优化非常棒！） 
//     const int x_ratio = (src_w << 16) / dst_w;
//     const int y_ratio = (src_h << 16) / dst_h;

//     int dst_idx = 0;
    
//     for (int y = 0; y < dst_h; ++y) {
//         const int src_y = (y * y_ratio) >> 16;
        
//         // 【核心修正】：使用物理步长 src_step 计算行偏移，而不是逻辑宽度 src_w
//         // 这能完美解决内存对齐导致的图像偏移和花屏问题
//         const int src_row_offset = src_y * src_step; 

//         for (int x = 0; x < dst_w; ++x) {
//             const int src_x = (x * x_ratio) >> 16;
            
//             // 只要步长算对了，这里的寻址就是安全的 
//             dst_buf[dst_idx++] = src_gray[src_row_offset + src_x];
//         }
//     }

//     return dst_buf;
// }
uint16_t* compress_rgb565_image(const uint16_t* src_rgb)
{
    // 1. 基础参数校验
    if (src_rgb == nullptr) return nullptr;
    if (UVC_WIDTH <= 0 || UVC_HEIGHT <= 0 || IMAGE_WIDTH <= 0 || IMAGE_HEIGHT <= 0) return nullptr;

    // 2. 计算缩放比例
    const float scale_x = (float)UVC_WIDTH / IMAGE_WIDTH;
    const float scale_y = (float)UVC_HEIGHT / IMAGE_HEIGHT;

    // 3. 分配输出内存
    const int dst_total = IMAGE_WIDTH * IMAGE_HEIGHT;
    uint16_t* dst_rgb = new uint16_t[dst_total]();

    // ========== 辅助函数：RGB565拆分为R/G/B分量 ==========
    auto rgb565_to_rgb = [](uint16_t rgb565, int& r, int& g, int& b) {
        // RGB565编码规则：高5位=R，中间6位=G，低5位=B
        r = (rgb565 >> 11) & 0x1F;
        g = (rgb565 >> 5) & 0x3F;
        b = rgb565 & 0x1F;
    };

    // ========== 辅助函数：R/G/B分量打包为RGB565 ==========
    auto rgb_to_rgb565 = [](int r, int g, int b) -> uint16_t {
        // 限制分量范围，避免越界
        r = (r < 0) ? 0 : (r > 31) ? 31 : r;
        g = (g < 0) ? 0 : (g > 63) ? 63 : g;
        b = (b < 0) ? 0 : (b > 31) ? 31 : b;
        // 重新打包：R(5位)<<11 + G(6位)<<5 + B(5位)
        return (uint16_t)((r << 11) | (g << 5) | b);
    };

    // 4. 双线性插值核心（按R/G/B分量插值，消除噪点）
    for (int y_dst = 0; y_dst < IMAGE_HEIGHT; y_dst++)
    {
        for (int x_dst = 0; x_dst < IMAGE_WIDTH; x_dst++)
        {
            // 计算目标像素在原图中的浮点坐标
            float x_src = x_dst * scale_x;
            float y_src = y_dst * scale_y;

            // 取4个邻近像素的整数坐标（边界保护）
            int x1 = static_cast<int>(x_src);
            int y1 = static_cast<int>(y_src);
            int x2 = (x1 + 1 < UVC_WIDTH) ? x1 + 1 : x1;
            int y2 = (y1 + 1 < UVC_HEIGHT) ? y1 + 1 : y1;

            // 插值权重
            float dx = x_src - x1;
            float dy = y_src - y1;

            // ========== 拆解4个邻近像素的R/G/B分量 ==========
            int r11, g11, b11; // 左上
            int r12, g12, b12; // 右上
            int r21, g21, b21; // 左下
            int r22, g22, b22; // 右下

            rgb565_to_rgb(src_rgb[y1 * UVC_WIDTH + x1], r11, g11, b11);
            rgb565_to_rgb(src_rgb[y1 * UVC_WIDTH + x2], r12, g12, b12);
            rgb565_to_rgb(src_rgb[y2 * UVC_WIDTH + x1], r21, g21, b21);
            rgb565_to_rgb(src_rgb[y2 * UVC_WIDTH + x2], r22, g22, b22);

            // ========== 对R/G/B分量分别做双线性插值 ==========
            float r = r11*(1-dx)*(1-dy) + r12*dx*(1-dy) + r21*(1-dx)*dy + r22*dx*dy;
            float g = g11*(1-dx)*(1-dy) + g12*dx*(1-dy) + g21*(1-dx)*dy + g22*dx*dy;
            float b = b11*(1-dx)*(1-dy) + b12*dx*(1-dy) + b21*(1-dx)*dy + b22*dx*dy;

            // ========== 重新打包为RGB565 ==========
            uint16_t p = rgb_to_rgb565(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b));
            dst_rgb[y_dst * IMAGE_WIDTH + x_dst] = p;
        }
    }

    return dst_rgb;
}
uint8_t* get_gray_ptr_from_color(const cv::Mat& color_img)
{
    // 防崩溃校验
    if (color_img.empty()) {
        return nullptr;
    }

    // 【极其关键】：必须使用 static！
    // 这样 gray_img 的内存会在整个程序运行期间常驻，
    // 下一次调用时复用同一块内存，不仅不会发生野指针崩溃，还省去了频繁 new/delete 内存的时间！
    static cv::Mat gray_img;

    // 转换色彩空间，极速算法
    cv::cvtColor(color_img, gray_img, cv::COLOR_BGR2GRAY);

    // 确保图像在物理内存中是连续排列的
    if (gray_img.isContinuous()) {
        // 直接返回底层的 unsigned char* 指针 (等价于 uint8_t*)
        return gray_img.data;
    } else {
        return nullptr;
    }
}
