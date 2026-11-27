#ifndef DISPLAY_H
#define DISPLAY_H

// 屏幕参数
#define WIDTH              320     // 屏幕宽度
#define HEIGHT             240     // 屏幕高度

// 定义颜色（RGB565 格式）
#define COLOR_RED          0xF800  // 1111100000000000
#define COLOR_GREEN        0x07E0  // 0000011111100000
#define COLOR_BLUE         0x001F  // 0000000000011111
#define COLOR_WHITE        0xFFFF  // 1111111111111111
#define COLOR_BLACK        0x0000  // 0000000000000000

// 欢迎界面
void welcome_display();

// 菜单界面
void menu_display(int menu_x, int menu_y, int list_num);

// 设置界面
void setting_display(int menu_y, int list_num);

// 打开framebuffer
int open_framebuffer();

// 关闭framebuffer
void close_framebuffer(int fb);

// 颜色字符串转二进制值
uint16_t get_color(const char *color);

// 打印像素
void print_pixel(int y, int x, const char* color);

// 灰度图转RGB565
uint16_t gray_to_rgb565(int gray_value);

// RGB888转RGB565
// void rgb888_to_rgb565(cv::Mat &frame, unsigned short* buffer);  // 已移除，不再需要

// 打印图像到屏幕指定位置（自动识别灰度/彩色）
void print_image(cv::Mat &frame, int y, int x);

void print_image_toushi(int setting);

void print_line_pixels();

void play_video(const std::string& video_path, int fps = 30, int x = 0, int y = 0);
void close_video();
void close_video();

void print_point_and_neighbors(int y, int x);

#endif // DISPLAY_H