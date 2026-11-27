#include "device_tft180.h"

TFT180::TFT180(void){}

TFT180::~TFT180()
{ 
    // 解除内存映射
    munmap(fbp, screensize);
    // 关闭设备
    close(fd);
}

int32_t TFT180::TFT180_init(void)
{
    fd = open(FB_TFT180_DEVICE, O_RDWR);
    if(fd < 0) {
        cerr << "Error: cannot open framebuffer device" << endl;
        return -1;
    } else {
        if(ioctl(fd, FBIOGET_FSCREENINFO, &fix_screeninfo)) {
            cerr << "Error reading fixed information" << endl;
            return -1;
        }
        if(ioctl(fd, FBIOGET_VSCREENINFO, &var_screeninfo)) {
            cerr << "Error reading variable information" << endl;
            return -1;
        }
    }

    /*  获取缓冲帧大小  */
    screensize = fix_screeninfo.smem_len;
    cout << "初始化屏幕完成\t" << "屏幕缓冲帧大小:" << screensize << endl;

    //映射帧缓冲内存
    fbp = (char *)mmap(nullptr, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(fbp == MAP_FAILED) {
        cerr << "Error: failed to map framebuffer memory" << endl;
        return -1;
    }

    clearScreen();

    return 0;
}

void TFT180::drawPoint(int x, int y, unsigned short color) 
{
    drawPixel(x, y, color);
}

void TFT180::clearScreen(void) 
{
    unsigned short *buffer = (unsigned short*)fbp;
    for(int i = 0; i < screensize / 2; i++) {
        buffer[i] = tft180_bgcolor;
    }
}

void TFT180::showChar(int x, int y, const char dat) 
{
    switch(tft180_display_font)
    {
        case TFT180_6X8_FONT:
        {
            const unsigned char *font = ascii_font_6x8[dat - 32];
            for(int col = 0; col < 6; col++) {
                unsigned char line = font[col];
                for(int row = 0; row < 8; row++) {
                    bool pixel = line & (0x80 >> row);
                    drawPixel(x + col, y + 7 - row, pixel ? tft180_pencolo : tft180_bgcolor);
                }

            }
        }
        break;
        case TFT180_8X16_FONT:
        {
            const unsigned char *font = ascii_font_8x16[dat - 32];
            for(int col = 0; col < 8; col++) {
                unsigned char upper = font[col];        // 上半部分（行0-7）
                unsigned char lower = font[col + 8];    // 下半部分（行8-15）
                for(int row = 0; row < 8; row++) {
                    bool pixel = upper & (0x80 >> row);
                    drawPixel(x + col, y + 7 - row, pixel ? tft180_pencolo : tft180_bgcolor);
                }
                for(int row = 0; row < 8; row++) {
                    bool pixel = lower & (0x80 >> row);
                    drawPixel(x + col, y + 15 - row, pixel ? tft180_pencolo : tft180_bgcolor);
                }
            }
        }
        break;
    }
}

void TFT180::showString(int x, int y, const char dat[])
{
    unsigned short j = 0;
    while('\0' != dat[j])
    {
        switch(tft180_display_font)
        {
            case TFT180_6X8_FONT:   showChar(x + 6 * j, y, dat[j]); break;
            case TFT180_8X16_FONT:  showChar(x + 8 * j, y, dat[j]); break;
            default: break;
        }
        j++;
    }
}

void TFT180::showInt(int x, int y, int dat, int num)
{
    int dat_temp = dat;
    int offset = 1;
    char data_buffer[12];

    memset(data_buffer, 0, 12);
    memset(data_buffer, ' ', num + 1);

    // 用来计算余数显示 123 显示 2 位则应该显示 23
    if(10 > num)
    {
        for(; 0 < num; num --)
        {
            offset *= 10;
        }
        dat_temp %= offset;
    }
    func_int_to_str(data_buffer, dat_temp);
    showString(x, y, (const char *)&data_buffer);
}

void TFT180::showUInt(int x, int y, int dat, int num)
{
    unsigned int dat_temp = dat;
    int offset = 1;
    char data_buffer[12];
    memset(data_buffer, 0, 12);
    memset(data_buffer, ' ', num);

    // 用来计算余数显示 123 显示 2 位则应该显示 23
    if(10 > num)
    {
        for(; 0 < num; num --)
        {
            offset *= 10;
        }
        dat_temp %= offset;
    }
    func_uint_to_str(data_buffer, dat_temp);
    showString(x, y, (const char *)&data_buffer);
}

void TFT180::showFloat (int x, int y, const double dat, unsigned char num, unsigned char pointnum)
{
    double dat_temp = dat;
    double offset = 1.0;
    char data_buffer[17];
    memset(data_buffer, 0, 17);
    memset(data_buffer, ' ', num + pointnum + 2);

    // 用来计算余数显示 123 显示 2 位则应该显示 23
    for(; 0 < num; num --)
    {
        offset *= 10;
    }
    dat_temp = dat_temp - ((int)dat_temp / (int)offset) * offset;
    func_double_to_str(data_buffer, dat_temp, pointnum);
    showString(x, y, (const char *)&data_buffer);
}

void TFT180::showCVImage(int x, int y, const cv::Mat& image, int target_width, int target_height)
{
    if(image.empty() || target_width <= 0 || target_height <= 0) return;

    // 非等比缩放处理
    cv::Mat resized_img;
    cv::resize(image, resized_img, cv::Size(target_width, target_height), 0, 0, cv::INTER_LINEAR);

    // 颜色空间转换 (BGR -> RGB565)
    cv::Mat rgb565;
    if(resized_img.channels() == 3) {
        cv::Mat rgb_img;
        cv::cvtColor(resized_img, rgb_img, cv::COLOR_BGR2RGB);
        rgb565.create(rgb_img.size(), CV_16UC1);

        for(int i = 0; i < rgb_img.rows; ++i) {
            const cv::Vec3b* ptr = rgb_img.ptr<cv::Vec3b>(i);
            unsigned short* dst = rgb565.ptr<unsigned short>(i);
            
            for(int j = 0; j < rgb_img.cols; ++j) {
                // RGB888转RGB565
                unsigned char r = ptr[j][0] >> 3;   // 5-bit
                unsigned char g = ptr[j][1] >> 2;   // 6-bit 
                unsigned char b = ptr[j][2] >> 3;   // 5-bit
                dst[j] = (r << 11) | (g << 5) | b;
            }
        }
    } else if(resized_img.channels() == 1) {
        // 灰度图直接转RGB565（伪彩色）
        rgb565.create(resized_img.size(), CV_16UC1);
        
        for(int i = 0; i < resized_img.rows; ++i) {
            const uchar* src = resized_img.ptr<uchar>(i);
            unsigned short* dst = rgb565.ptr<unsigned short>(i);
            
            for(int j = 0; j < resized_img.cols; ++j) {
                uchar gray = src[j];
                // 将灰度值均匀分配到RGB通道（模拟灰度显示）
                unsigned short r = (gray >> 3) & 0x1F;  // 5-bit红
                unsigned short g = (gray >> 2) & 0x3F;  // 6-bit绿
                unsigned short b = (gray >> 3) & 0x1F;  // 5-bit蓝
                dst[j] = (r << 11) | (g << 5) | b;
            }
        }
    } else {
        std::cerr << "Unsupported image format" << std::endl;
        return;
    }

    // 边界裁剪处理
    const int screen_width = var_screeninfo.xres;
    const int screen_height = var_screeninfo.yres;
    
    const int draw_width = std::min(target_width, screen_width - x);
    const int draw_height = std::min(target_height, screen_height - y);
    
    if(draw_width <= 0 || draw_height <= 0) return;

    // 直接操作帧缓冲内存
    for(int iy = 0; iy < draw_height; ++iy) {
        const unsigned short* src = rgb565.ptr<unsigned short>(iy);
        unsigned short* dst = reinterpret_cast<unsigned short*>(
            fbp + (y + iy + var_screeninfo.yoffset) * fix_screeninfo.line_length
        ) + x + var_screeninfo.xoffset;
        
        std::memcpy(dst, src, draw_width * sizeof(unsigned short));
    }
}

void TFT180::drawPixel(int x, int y, unsigned short color) 
{
    if(x >= 0 && x < var_screeninfo.xres && y >= 0 && y < var_screeninfo.yres) {
        long offset = (x + var_screeninfo.xoffset) * (var_screeninfo.bits_per_pixel / 8) + 
                    (y + var_screeninfo.yoffset) * fix_screeninfo.line_length;
        *((unsigned short*)(fbp + offset)) = color;
    }
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
void TFT180::func_int_to_str (char *str, int number)
{
    unsigned char data_temp[16];                                                        // 缓冲区
    unsigned char bit = 0;                                                              // 数字位数
    int number_temp = 0;

    do
    {
        if(NULL == str)
        {
            break;
        }

        if(0 > number)                                                          // 负数
        {
            *str ++ = '-';
            number = -number;
        }
        else if(0 == number)                                                    // 或者这是个 0
        {
            *str = '0';
            break;
        }

        while(0 != number)                                                      // 循环直到数值归零
        {
            number_temp = number % 10;
            data_temp[bit ++] = func_abs(number_temp);                          // 倒序将数值提取出来
            number /= 10;                                                       // 削减被提取的个位数
        }
        while(0 != bit)                                                         // 提取的数字个数递减处理
        {
            *str ++ = (data_temp[bit - 1] + 0x30);                              // 将数字从倒序数组中倒序取出 变成正序放入字符串
            bit --;
        }
    }while(0);
}

void TFT180::func_uint_to_str (char *str, unsigned int number)
{
    char data_temp[16];                                                         // 缓冲区
    unsigned char bit = 0;                                                      // 数字位数

    do
    {
        if(NULL == str)
        {
            break;
        }

        if(0 == number)                                                         // 这是个 0
        {
            *str = '0';
            break;
        }

        while(0 != number)                                                      // 循环直到数值归零
        {
            data_temp[bit ++] = (number % 10);                                  // 倒序将数值提取出来
            number /= 10;                                                       // 削减被提取的个位数
        }
        while(0 != bit)                                                         // 提取的数字个数递减处理
        {
            *str ++ = (data_temp[bit - 1] + 0x30);                              // 将数字从倒序数组中倒序取出 变成正序放入字符串
            bit --;
        }
    }while(0);
}

void TFT180::func_double_to_str (char *str, double number, unsigned char point_bit)
{
    int data_int = 0;                                                           // 整数部分
    int data_float = 0.0;                                                       // 小数部分
    int data_temp[12];                                                          // 整数字符缓冲
    int data_temp_point[9];                                                     // 小数字符缓冲
    unsigned char bit = point_bit;                                              // 转换精度位数

    do
    {
        if(NULL == str)
        {
            break;
        }

        // 提取整数部分
        data_int = (int)number;                                                 // 直接强制转换为 int
        if(0 > number)                                                          // 判断源数据是正数还是负数
        {
            *str ++ = '-';
        }
        else if(0.0 == number)                                                  // 如果是个 0
        {
            *str ++ = '0';
            *str ++ = '.';
            *str = '0';
            break;
        }

        // 提取小数部分
        number = number - data_int;                                             // 减去整数部分即可
        while(bit --)
        {
            number = number * 10;                                               // 将需要的小数位数提取到整数部分
        }
        data_float = (int)number;                                               // 获取这部分数值

        // 整数部分转为字符串
        bit = 0;
        do
        {
            data_temp[bit ++] = data_int % 10;                                  // 将整数部分倒序写入字符缓冲区
            data_int /= 10;
        }while(0 != data_int);
        while(0 != bit)
        {
            *str ++ = (func_abs(data_temp[bit - 1]) + 0x30);                    // 再倒序将倒序的数值写入字符串 得到正序数值
            bit --;
        }

        // 小数部分转为字符串
        if(point_bit != 0)
        {
            bit = 0;
            *str ++ = '.';
            if(0 == data_float)
                *str = '0';
            else
            {
                while(0 != point_bit)                                           // 判断有效位数
                {
                    data_temp_point[bit ++] = data_float % 10;                  // 倒序写入字符缓冲区
                    data_float /= 10;
                    point_bit --;
                }
                while(0 != bit)
                {
                    *str ++ = (func_abs(data_temp_point[bit - 1]) + 0x30);      // 再倒序将倒序的数值写入字符串 得到正序数值
                    bit --;
                }
            }
        }
    }while(0);
}
#pragma GCC diagnostic pop

