#include "zf_common_headfile.h"
#include <unistd.h>
#include "gif_lib.h"
#include <iomanip>
#include "temperature.h"

#define FB_PATH "/dev/fb0"  // framebuffer 设备路径


// 映射的 framebuffer 内存指针
uint16_t* framebuffer = nullptr;

extern int list_lenth[9];
extern int setting_thing[20];
extern int add_sum;
extern int left_line[1000][2];
extern int right_line[1000][2];
extern int mid_line_from_left[1000][2];
extern int mid_line_from_right[1000][2];
extern int left_corner[100][2];
extern int right_corner[100][2];
extern uint8_t *PerImg_ip[240][320];
extern int left_lenth;
extern int mid_lenth_left;
extern int mid_lenth_right;
extern int right_lenth;
extern uint8_t *PerImg_Color_ip_1[RESULT_ROW][RESULT_COL];
extern uint8_t *PerImg_Color_ip[RESULT_ROW][RESULT_COL];
extern int barrier_point[2];
// 主菜单
char main_menu[11][20] = {
    " Main Menu ",
    " ",
    " Setting 1 ",
    " Setting 2 ",
    " Setting 3 ",
    " Display ",
    " Start ",
    " Test ",
    " Exit ",
};

// 设置菜单1 - 标题翻译，内容保持英文
char setting_menu1[11][20] = {
    " Setting 1 ",
    " ",
    " Slow Speed ",
    " Mid Speed ",
    " Fast Speed ",
    " Barrier Speed ",
    " wd ",
    " Barrier Start Num ",
    " Barrier End Num "
};

// 设置菜单2 - 标题翻译，内容保持英文
char setting_menu2[11][20] = {
    " Setting 2 ",
    " ",
    " barrier_hold ",
    " Curve Far ",
    " Far Num ",
    " Zebra Hold ",
    " PWM Fan ",
    " Round Num ",
    " Image Scale"

};

// 设置菜单3 - 标题翻译，内容保持英文
char setting_menu3[11][20] = {
    " Setting 3 ",
    " ",
    " kp dif ",
    " kd dif ",
    " gp ",
    " kp ",
    " kd ",
    " Exposure ",
    " Auto Exposure "
};

// 显示菜单
char display_menu[11][20] = {
    " Display ",
    " ",
    " Binarization ",
    " Grayscale ",
    " Color Perspective ",
    " Color ",
    " Binarization Pro",
    " Grayscale Pro",
    " "
};

// 开始菜单
char start_menu[11][20] = {
    " Start ",
    " ",
    " Save ",
    " Test ",
    " Slow ",
    " Stable ",
    " Fast",
    " Fly",
    " Rocket "

};
char test_menu[11][20] = {
    " Test ",
    " ",
    " Capture ",
    " Camera Setting ",
    " Uart Test ",
    " ",
    " ",
    " ",
    " "

};





char setting_list[20][20] = {"Slow Speed" , "Mid Speed", "Fast Speed", "Barrier Speed", "wd", "Barrier Start Num", "Barrier End Num"};

char setting_list2[20][20] = {"Barrier Hold", "Curve Far", "Far_num", "Zebra Hold", "PWM Fan", "Round Num", "Image Scale"};

char setting_list3[20][20] = {"kp dif", "kd dif", "gp", "kp", "kd"};

// 获取字符串的有效长度（去除首尾空格）
int get_effective_length(const char* str, int max_len) {
    int start = 0, end = max_len - 1;
    
    // 找第一个非空格字符
    while (start < max_len && str[start] == ' ') start++;
    
    // 找最后一个非空格字符
    while (end >= 0 && str[end] == ' ') end--;
    
    if (start > end) return 0;
    return end - start + 1;
}

// 获取字符串的起始位置（第一个非空格字符）
int get_start_pos(const char* str, int max_len) {
    int start = 0;
    while (start < max_len && str[start] == ' ') start++;
    return start;
}

// 获取字符串的结束位置（最后一个非空格字符）
int get_end_pos(const char* str, int max_len) {
    int end = max_len - 1;
    while (end >= 0 && str[end] == ' ') end--;
    return end;
}

void welcome_display(){
    Mat frame = imread("/home/root/320.png");
    if (frame.empty()) {
        cerr << "无法加载图片！" << std::endl;
        return;
    }
    print_image(frame, 0, 0);
}

void menu_display(int menu_x, int menu_y, int list_num) {
    //system("clear");

    char (*menu)[20];
    int menu_size = 0;  // 菜单实际行数

    if (menu_x == 0) {
        menu = main_menu;
        menu_size = 10;
    } else if (menu_y == 1) {
        menu = setting_menu1;
        menu_size = 10;
    } else if (menu_y == 2) {
        menu = setting_menu2;
        menu_size = 10;
    } else if (menu_y == 3) {
        menu = setting_menu3;
        menu_size = 10;
    } else if (menu_y == 4) {
        menu = display_menu;
        menu_size = 10;
    } else if (menu_y == 5) {
        menu = start_menu;
        menu_size = 10;
    } else if (menu_y == 6) {
        menu = test_menu;
        menu_size = 10;
    } else {
        return;
    }

    // 按实际有效大小显示菜单
    for (int i = 0; i < menu_size; i++) {
        int str_len = strlen(menu[i]);
        int start = get_start_pos(menu[i], str_len);
        int end = get_end_pos(menu[i], str_len);
        
        if (i == list_num + 1) {
            // 选中项：反色显示，左右各扩展一个字符
            int highlight_start = (start > 0) ? start - 1 : 0;
            int highlight_end = (end < str_len - 1) ? end + 1 : str_len - 1;
            
            // 打印选中行
            for (int j = 0; j < str_len; j++) {
                if (j == highlight_start) {
                    printf("\033[7m");  // 开始反色
                }
                putchar(menu[i][j]);
                if (j == highlight_end) {
                    printf("\033[0m");  // 结束反色
                }
            }
        } else {
            // 普通项：按实际有效内容显示
            for (int j = 0; j < str_len; j++) {
                putchar(menu[i][j]);
            }
        }
        putchar('\n');
    }
    
    cout << fixed << setprecision(2) << get_battery_voltage() << "V  "  << fixed << setprecision(2) << get_cpu_temperature() << "°C" << endl;
    fflush(stdout);
}

void setting_display(int menu_y, int list_num){
    system("clear");
    
    switch (menu_y) {
        case 1:
            printf("%s", setting_list[list_num - 1]);
            break;
        case 2:
            printf("%s", setting_list2[list_num - 1]);
            break;
        case 3:
            printf("%s", setting_list3[list_num - 1]);
            break;
        default:
            break;
    }

    printf("\n\n");
    printf("%d", setting_thing[list_num]);
    printf("\n\n");
    printf("X%d", add_sum);
    printf("\n");
}

int open_framebuffer() {
    int fb = open(FB_PATH, O_RDWR);
    if (fb == -1) {
        perror("Failed to open framebuffer");
        return -1;
    }

    framebuffer = (uint16_t*) mmap(NULL, WIDTH * HEIGHT * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (framebuffer == MAP_FAILED) {
        perror("Failed to map framebuffer");
        close(fb);
        return -1;
    }

    return fb;
}

void close_framebuffer(int fb) {
    if (framebuffer) {
        munmap(framebuffer, WIDTH * HEIGHT * 2);
    }
    close(fb);
}

uint16_t get_color(const char *color) {
    if (strcmp(color, "red") == 0) {
        return COLOR_RED;
    } else if (strcmp(color, "green") == 0) {
        return COLOR_GREEN;
    } else if (strcmp(color, "blue") == 0) {
        return COLOR_BLUE;
    } else if (strcmp(color, "white") == 0) {
        return COLOR_WHITE;
    } else if (strcmp(color, "black") == 0) {
        return COLOR_BLACK;
    } else {
        return COLOR_BLACK;
    }
}

void print_pixel(int y, int x, const char* color) {
    if (y < 0 || y >= HEIGHT || x < 0 || x >= WIDTH) {
        return;
    }

    uint16_t color_value = get_color(color);
    int pixel_index = y * WIDTH + x;
    framebuffer[pixel_index] = color_value;
}

uint16_t gray_to_rgb565(int gray_value) {
    int r = (gray_value * 31) / 255;
    int g = (gray_value * 63) / 255;
    int b = (gray_value * 31) / 255;
    return (r << 11) | (g << 5) | b;
}

inline uint16_t bgr_to_rgb565(uint8_t b, uint8_t g, uint8_t r) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void print_image(Mat &frame, int y, int x) {
    if (framebuffer == nullptr) return;
    if (frame.empty()) return;

    int rows = frame.rows;
    int cols = frame.cols;
    int channels = frame.channels();

    for (int i = 0; i < rows; i++) {
        int screen_y = y + i;
        if (screen_y < 0 || screen_y >= HEIGHT) continue;

        // YUYV 需要按字节访问，提前取行指针
        uint8_t *row_data = nullptr;
        if (channels == 2) {
            row_data = frame.ptr<uint8_t>(i);
        }

        for (int j = 0; j < cols; j++) {
            int screen_x = x + j;
            if (screen_x < 0 || screen_x >= WIDTH) continue;

            uint16_t color_value;

            if (channels == 1) {
                uint8_t gray = frame.at<uint8_t>(i, j);
                color_value = gray_to_rgb565(gray);
            } else if (channels == 2) {
                // YUYV 格式 (CV_8UC2): 宏像素 [Y0, U, Y1, V]
                int macro_x = j / 2;
                int is_odd  = j % 2;
                int Y = is_odd ? row_data[macro_x * 4 + 2]   // Y1
                               : row_data[macro_x * 4 + 0];  // Y0
                int U = row_data[macro_x * 4 + 1];
                int V = row_data[macro_x * 4 + 3];

                // BT.601 YUV → RGB
                int C = Y - 16, D = U - 128, E = V - 128;
                int r = (298 * C + 409 * E + 128) >> 8;
                int g = (298 * C - 100 * D - 208 * E + 128) >> 8;
                int b = (298 * C + 516 * D + 128) >> 8;
                r = r < 0 ? 0 : (r > 255 ? 255 : r);
                g = g < 0 ? 0 : (g > 255 ? 255 : g);
                b = b < 0 ? 0 : (b > 255 ? 255 : b);
                color_value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            } else {
                Vec3b bgr = frame.at<Vec3b>(i, j);
                color_value = bgr_to_rgb565(bgr[0], bgr[1], bgr[2]);
            }

            framebuffer[screen_y * WIDTH + screen_x] = color_value;
        }
    }
}

void print_image_toushi(int setting) {
    if (framebuffer == nullptr) return;

    if (setting == 0) {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int gray_value = get_image_sum(y, x);
                uint16_t color_value = gray_to_rgb565(gray_value);
                framebuffer[y * WIDTH + x] = color_value;
            }
        }
    } else {
        for (int y = 0; y < RESULT_ROW; y++) {
            for (int x = 0; x < RESULT_COL; x++) {
                uint8_t *ptr = PerImg_Color_ip[y][x];
                if (ptr == nullptr) continue;

                // 解码 YUYV 指针（与 cut_image 一致）
                bool is_odd = ((uintptr_t)ptr & 1) != 0;
                uint8_t *base = (uint8_t *)((uintptr_t)ptr & ~(uintptr_t)1);

                int Y = is_odd ? base[2] : base[0];
                int U = base[1];
                int V = base[3];

                // BT.601 YUV → RGB565
                int C = Y - 16, D = U - 128, E = V - 128;
                int r = (298 * C + 409 * E + 128) >> 8;
                int g = (298 * C - 100 * D - 208 * E + 128) >> 8;
                int b = (298 * C + 516 * D + 128) >> 8;
                r = r < 0 ? 0 : (r > 255 ? 255 : r);
                g = g < 0 ? 0 : (g > 255 ? 255 : g);
                b = b < 0 ? 0 : (b > 255 ? 255 : b);

                uint16_t color_value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

                if (y < HEIGHT && x < WIDTH) {
                    framebuffer[y * WIDTH + x] = color_value;
                }
            }
        }
    }
}

void print_point_and_neighbors(int y, int x) {
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            print_pixel(y + dy, x + dx, "white");
        }
    }
}

void print_line_pixels() {
    for (int i = 0; i <= left_lenth; i++) {
        print_pixel(left_line[i][0], left_line[i][1], "blue");
    }
    for (int i = 0; i <= right_lenth; i++) {
        print_pixel(right_line[i][0], right_line[i][1], "red");
    }
    for (int i = 0; i <= mid_lenth_left; i++) {
        print_pixel(mid_line_from_left[i][0], mid_line_from_left[i][1], "green");
    }
    for (int i = 0; i <= mid_lenth_right; i++) {
        print_pixel(mid_line_from_right[i][0], mid_line_from_right[i][1], "green");
    }
    for(int i = 1; i <= left_corner[0][0]; i++) {
        print_point_and_neighbors(left_line[left_corner[i][0]][0], left_line[left_corner[i][0]][1]);
    }
    for(int i = 1; i <= right_corner[0][0]; i++) {
        print_point_and_neighbors(right_line[right_corner[i][0]][0], right_line[right_corner[i][0]][1]);
    }
    print_point_and_neighbors(barrier_point[0], barrier_point[1]);
}

struct GifFrame {
    cv::Mat image;       // BGR格式的帧图像
    int delay_ms;        // 帧延迟（毫秒）
};

struct VideoContext {
    cv::VideoCapture cap;
    cv::Mat frame;
    
    // gif相关
    std::vector<GifFrame> gif_frames;
    int gif_current = 0;
    
    bool is_gif = false;
    bool is_playing = false;
    Pit_timer* timer = nullptr;
    int display_x = 0;
    int display_y = 0;
};

static VideoContext g_video_ctx;

// 将giflib的ColorMapObject转为BGR颜色
static cv::Vec3b get_gif_color(const ColorMapObject* colorMap, int colorIdx) {
    if (colorMap == nullptr || colorIdx < 0 || colorIdx >= colorMap->ColorCount) {
        return cv::Vec3b(0, 0, 0);
    }
    GifColorType& c = colorMap->Colors[colorIdx];
    return cv::Vec3b(c.Blue, c.Green, c.Red);  // BGR
}

// 加载gif所有帧
static bool load_gif(const std::string& path) {
    GifFileType* gif = DGifOpenFileName(path.c_str(), nullptr);
    if (!gif) {
        std::cerr << "无法打开GIF文件: " << path << std::endl;
        return false;
    }
    
    if (DGifSlurp(gif) != GIF_OK) {
        std::cerr << "无法解析GIF文件: " << path << std::endl;
        DGifCloseFile(gif, nullptr);
        return false;
    }
    
    int frame_count = gif->ImageCount;
    int width = gif->SWidth;
    int height = gif->SHeight;
    
    std::cout << "GIF: " << width << "x" << height << ", " << frame_count << " 帧" << std::endl;
    
    // 全局颜色表
    const ColorMapObject* globalColorMap = gif->SColorMap;
    
    // canvas用于合成（处理dispose方式）
    cv::Mat canvas(height, width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    // 背景色
    cv::Vec4b bgColor(0, 0, 0, 0);
    if (globalColorMap) {
        auto bgr = get_gif_color(globalColorMap, gif->SBackGroundColor);
        bgColor = cv::Vec4b(bgr[0], bgr[1], bgr[2], 255);
    }
    canvas.setTo(bgColor);
    
    g_video_ctx.gif_frames.clear();
    
    for (int i = 0; i < frame_count; i++) {
        SavedImage& saved = gif->SavedImages[i];
        GraphicsControlBlock gcb;
        
        // 获取帧延迟
        int delay = 100;  // 默认100ms
        // 遍历扩展块查找GCB
        for (int j = 0; j < saved.ExtensionBlockCount; j++) {
            if (saved.ExtensionBlocks[j].Function == 0xF9) {
                if (DGifExtensionToGCB(0xF9,
                    (const unsigned char*)saved.ExtensionBlocks[j].Bytes,
                    &gcb) == GIF_OK) {
                    delay = gcb.DelayTime * 10;  // gif延迟单位是1/100秒
                    if (delay < 20) delay = 100;  // 过小则用默认值
                }
                break;
            }
        }
        
        // 获取帧的颜色表
        const ColorMapObject* colorMap = globalColorMap;
        if (saved.ImageDesc.ColorMap) {
            colorMap = saved.ImageDesc.ColorMap;
        }
        
        int left = saved.ImageDesc.Left;
        int top = saved.ImageDesc.Top;
        int fw = saved.ImageDesc.Width;
        int fh = saved.ImageDesc.Height;
        
        // 获取dispose方式和透明色
        int dispose = 0;
        int transparent = -1;
        for (int j = 0; j < saved.ExtensionBlockCount; j++) {
            if (saved.ExtensionBlocks[j].Function == 0xF9) {
                GraphicsControlBlock tmp;
                if (DGifExtensionToGCB(0xF9,
                    (const unsigned char*)saved.ExtensionBlocks[j].Bytes,
                    &tmp) == GIF_OK) {
                    dispose = tmp.DisposalMode;
                    transparent = tmp.TransparentColor;
                }
                break;
            }
        }
        
        // 将帧数据写入canvas
        for (int y = 0; y < fh; y++) {
            for (int x = 0; x < fw; x++) {
                int colorIdx = saved.RasterBits[y * fw + x];
                
                // 跳过透明像素
                if (colorIdx == transparent) continue;
                
                int cy = top + y;
                int cx = left + x;
                if (cy < 0 || cy >= height || cx < 0 || cx >= width) continue;
                
                auto bgr = get_gif_color(colorMap, colorIdx);
                canvas.at<cv::Vec4b>(cy, cx) = cv::Vec4b(bgr[0], bgr[1], bgr[2], 255);
            }
        }
        
        // 保存当前帧（转BGR）
        GifFrame gf;
        cv::Mat bgr_frame;
        cv::cvtColor(canvas, bgr_frame, cv::COLOR_BGRA2BGR);
        gf.image = bgr_frame.clone();
        gf.delay_ms = delay;
        g_video_ctx.gif_frames.push_back(gf);
        
        // 处理dispose方式，为下一帧准备canvas
        if (dispose == 2) {  // 恢复为背景色
            for (int y = top; y < top + fh && y < height; y++) {
                for (int x = left; x < left + fw && x < width; x++) {
                    canvas.at<cv::Vec4b>(y, x) = bgColor;
                }
            }
        }
        // dispose == 0 或 1: 保留当前canvas
    }
    
    DGifCloseFile(gif, nullptr);
    return !g_video_ctx.gif_frames.empty();
}

// 定时器回调函数，用于显示下一帧
void video_timer_callback() {
    if (!g_video_ctx.is_playing) {
        return;
    }
    
    cv::Mat display_frame;
    
    if (g_video_ctx.is_gif) {
        if (g_video_ctx.gif_frames.empty()) return;
        
        auto& gf = g_video_ctx.gif_frames[g_video_ctx.gif_current];
        display_frame = gf.image;
        
        // 移动到下一帧
        g_video_ctx.gif_current++;
        if (g_video_ctx.gif_current >= (int)g_video_ctx.gif_frames.size()) {
            g_video_ctx.gif_current = 0;  // 循环播放
        }
    } else {
        if (!g_video_ctx.cap.isOpened()) return;
        
        if (!g_video_ctx.cap.read(display_frame)) {
            // 视频结束，循环播放
            g_video_ctx.cap.set(cv::CAP_PROP_POS_FRAMES, 0);
            if (!g_video_ctx.cap.read(display_frame)) {
                std::cerr << "Cannot read video frame" << std::endl;
                g_video_ctx.is_playing = false;
                return;
            }
        }
    }
    
    // 显示帧
    if (!display_frame.empty()) {
        print_image(display_frame, g_video_ctx.display_y, g_video_ctx.display_x);
    }
}

void play_video(const std::string& video_path, int fps, int x, int y) {
    // 判断文件是否为gif
    std::string ext = video_path.substr(video_path.find_last_of('.'));
    for (auto& c : ext) c = tolower(c);
    
    g_video_ctx.is_gif = (ext == ".gif");
    g_video_ctx.display_x = x;
    g_video_ctx.display_y = y;
    g_video_ctx.gif_current = 0;
    g_video_ctx.is_playing = true;
    
    uint32_t interval_ms;
    
    if (g_video_ctx.is_gif) {
        // 加载gif
        if (!load_gif(video_path)) {
            std::cerr << "Cannot load GIF file: " << video_path << std::endl;
            return;
        }
        
        // 使用第一帧的延迟作为定时器间隔
        interval_ms = g_video_ctx.gif_frames[0].delay_ms;
    } else {
        // 打开视频
        g_video_ctx.cap.open(video_path);
        if (!g_video_ctx.cap.isOpened()) {
            std::cerr << "Cannot open video file: " << video_path << std::endl;
            return;
        }
        
        //int video_width = static_cast<int>(g_video_ctx.cap.get(cv::CAP_PROP_FRAME_WIDTH));
        //int video_height = static_cast<int>(g_video_ctx.cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        double video_fps = g_video_ctx.cap.get(cv::CAP_PROP_FPS);
    
        
        if (fps <= 0 && video_fps > 0) {
            fps = static_cast<int>(video_fps);
        } else if (fps <= 0) {
            fps = 30;
        }
        
        interval_ms = 1000 / fps;
    }
    
    // 创建定时器
    g_video_ctx.timer = pit_ms_init(interval_ms, video_timer_callback);
}

void close_video() {
    if (!g_video_ctx.is_playing) return;
    
    g_video_ctx.is_playing = false;
    
    // 停止定时器
    if (g_video_ctx.timer) {
        pit_ms_delete(g_video_ctx.timer);
        g_video_ctx.timer = nullptr;
    }
    
    if (!g_video_ctx.is_gif) {
        g_video_ctx.cap.release();
    }
    g_video_ctx.gif_frames.clear();
    
}