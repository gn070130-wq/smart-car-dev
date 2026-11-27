/*
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢋⡥⠀⢿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣾⡇⡆⢸
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢻⣿⡏⢰⣿⢷⠃⠀
⣿⣿⣿⣿⢟⡍⠥⣭⣍⠻⡿⢋⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣯⠻⣿⣿⣿⣏⠁⢾⡇⠘⠀⠀
⣿⣿⣿⣟⠷⣶⢿⣾⣿⣧⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡙⣿⣿⣧⡆⠘⣧⠀⠀⣤
⣿⣿⣿⣿⣦⣙⣋⣹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠐⠀⠹⠻⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡜⣿⡋⠅⠀⢻⡄⠀⢻
⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣿⣿⣿⣿⡇⣿⣿⣿⣿⣿⣿⠀⢩⣐⠐⢀⡬⠉⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡘⣿⣆⠀⠈⣧⠀⢸
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡧⠸⢿⣿⣿⣿⣿⡆⢢⣤⣤⣤⣴⣷⣧⣄⠙⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⡉⢿⣆⠀⢻⠀⠀
⣿⡟⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠄⠀⠘⣿⣿⣿⣿⡇⢺⣿⣿⣿⣿⣿⣿⣿⣷⣄⡈⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⢰⠈⣿⡄⠸⡆⠀
⣿⣿⠂⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡅⠀⢠⣶⡌⢻⣿⣿⣿⠸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣤⣀⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡏⠸⠀⢘⠣⠀⠿⠀
⣿⣿⢃⣾⣿⣿⡿⣿⣿⣿⣿⣿⣿⡟⣿⡁⢀⣾⣿⣿⣦⡙⠻⣿⣇⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⡙⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠐⢸⡄⠈⠀
⣿⣿⣿⠏⠻⡟⠡⠌⢻⣿⣿⣿⣿⡇⢻⡇⢸⣿⣿⣿⣿⣿⣦⢈⠙⠌⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠟⠃⠀⠈⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠁⠀⠀⠀⡼⢰⠀⢧
⣿⣿⣿⣘⢓⣀⣩⡥⢚⣉⡛⠿⣿⡇⠘⡇⡼⢿⣿⣿⣿⣿⣿⣿⣷⣦⣼⣿⣿⣿⣿⣿⣿⠿⠛⠉⠁⣀⣤⣴⣶⣶⠀⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡏⠀⠀⠀⠀⢃⣾⡄⣼
⣿⣿⣿⣿⣿⣿⣿⠠⣡⡸⢠⡄⠸⡇⠀⢠⣃⡀⠉⠙⠻⠿⣿⣿⣿⣿⣿⣿⣿⣿⡟⠋⠀⢠⣴⣶⣿⣿⣿⣿⣿⣿⡆⣾⡟⢿⣿⣿⣿⣿⣿⣿⣟⠂⠀⠀⠀⠀⢀⣿⣷⣿
⣿⣿⣿⣿⣿⣿⣿⣷⣝⠳⠆⠐⣰⣷⠀⠸⣿⣿⣷⣦⡤⠀⢀⣿⣿⣿⣿⣿⣿⣿⣷⣶⣤⣄⡀⠉⠛⠿⢿⣿⣿⡿⢠⣿⡝⣌⢿⣿⣿⣿⣿⣿⠏⠀⠀⠀⠀⠀⠈⠛⠻⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⠀⣿⣿⡿⠋⣀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠦⣤⣀⠈⢙⠃⣾⣿⣷⣼⡆⢻⣿⣿⣿⡇⠀⠀⢰⣦⠀⢠⣤⠀⠀⣿
⣿⣿⣿⣿⣿⣿⣿⠛⡙⢿⣿⣿⣿⣿⡧⠀⡿⢁⠔⣾⣿⣿⣿⣿⣿⠿⠿⣿⣿⡿⠟⢻⣿⣿⣿⣿⣷⣍⣛⡻⠂⣸⣿⣿⣿⠃⠀⠈⢿⡿⢋⠤⠒⢀⠀⠀⠀⠈⡁⠀⣸⣿
⣿⣿⣿⣿⣿⣿⣿⣦⣰⣾⢻⣿⡿⠻⣿⠀⠡⣡⣾⣯⣿⢠⣶⣴⣶⣾⣷⣶⣶⣶⣿⡇⣿⣿⣿⣿⣿⣿⣿⠋⣼⣿⣿⣿⠋⠀⠀⠀⠀⠁⡏⠀⠀⢁⣴⣶⣟⠔⠀⠁⣿⣿
⣿⣿⣿⡿⠟⣿⣿⣿⣿⣿⣦⠟⣀⠙⠒⣠⡸⣿⣿⣿⣿⡌⢿⣿⣿⣿⣿⣿⣿⣿⣿⡇⣽⣿⣿⣿⣿⡿⢃⣾⣿⣿⡿⢋⣴⠀⠀⢀⣼⡀⣇⢠⣾⣿⣿⣿⣿⣷⣾⣷⣦⠙
⣿⡟⣡⠞⣉⠠⠀⡀⠉⠋⠿⢦⡘⢧⣠⠠⠄⢻⣿⣿⣿⣿⣌⠻⣿⣿⣿⡿⢋⡙⠙⣰⣿⣿⣿⣿⣿⡃⠛⢛⣋⣡⣴⣿⣿⠗⣀⢿⠃⠁⠘⢦⡉⠛⠻⣿⣿⣿⡿⠏⣡⣼
⣿⡶⠃⡜⠹⣧⡘⠈⠂⠄⠆⠀⠙⢷⣌⣌⠂⠀⠈⠻⠟⠿⠿⠷⣦⣭⣭⢑⠃⠙⡁⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠁⠸⠟⠈⠀⠀⠀⠀⠙⠦⣄⣀⣀⡀⠀⣰⣿⣿
⣿⠁⠀⠀⠓⠈⢿⠀⠈⠰⢄⢀⠀⠀⠻⣷⠀⠀⠀⢀⠀⠀⠀⠀⠠⠩⣉⡈⠛⠛⠁⠈⠉⢉⣩⣭⣬⣭⠙⣿⣿⠟⠁⠀⠡⠀⠀⠀⠀⠀⠀⣆⠶⠶⠀⢀⣥⣴⣿⣿⣿⣿
⡇⠂⠀⠀⠀⠀⠀⢠⠀⠰⠀⠠⠁⠀⠀⠙⠧⠀⠈⠉⠀⠀⠀⢀⣷⡀⠀⣈⣉⠀⠀⠀⣶⣶⠶⠤⠤⠍⠘⠋⠁⠀⣠⠆⠀⠀⠀⠀⠠⠖⠋⡉⢥⣶⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣷⣄⣀⠐⠀⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠁⠴⣿⡿⢀⣼⣿⡿⠷⠀⡰⢋⢡⣶⣿⡇⢏⣵⣶⣴⡄⠀⠀⠀⠀⠀⠊⣀⣤⣄⠰⠆⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣷⣶⣶⣦⣢⣤⣤⣀⠀⠀⠀⠀⡜⣰⣿⣿⡶⢈⠄⣼⣿⣿⠀⣵⣾⣞⣛⡻⢿⣿⠀⣾⣿⣿⣿⣷⣄⠀⠀⠀⢠⣶⣯⣍⣙⣁⣰⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣄⡘⠰⡿⣿⡿⠃⢃⣾⣿⣿⡟⢸⣑⡀⡈⠉⠙⢠⣿⢘⣿⢿⣿⣿⣿⡿⠿⠟⣠⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣌⠛⠀⢣⣿⣿⣿⣿⠇⣼⣷⢗⣹⣿⣶⣿⣿⠀⢿⡌⢿⣿⣟⠁⣀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠁⠂⣻⣿⣿⣿⡿⢀⣿⣿⣿⣿⣿⣿⣿⣿⣷⠈⢷⣄⠀⣠⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢿⡟⠁⣠⣾⣿⣿⣿⣿⠁⣸⣿⣿⣿⣿⣿⣿⣿⣿⣷⣂⢀⣀⡠⢾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⡿⣻⣿⣿⡿⠛⣊⡀⠀⢐⣶⣿⣿⣿⣿⣿⣿⠇⢀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⡟⢀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣧⡙⢋⡠⠂⣂⠬⣩⠍⡙⢿⣿⠿⠛⠉⠁⠀⢀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⡟⠉⣰⢠⣶⣿⡁⢘⣥⠾⠋⢀⠁⠀⠀⢀⠀⣷⣉⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣘⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣇⠡⠡⡸⠿⣋⠲⠛⣉⡤⠚⠀⠀⠀⡀⠨⣶⡸⣿⣝⡿⠿⠟⣛⣋⣭⣥⣤⣤⣍⣙⠛⠿⡌⢽⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣷⣌⡓⠘⠙⠷⠋⠁⠀⠀⣠⠄⡀⠁⠀⠸⢵⠬⠴⠒⠻⠋⠙⠁⠉⢁⠈⠀⠀⠉⠭⠵⢨⣉⣘⣩⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
*/

#include "zf_common_headfile.h"
#include "pwm.h"
#include "cst816.h"
#include "my_timeer.h"
#include "web_service_main.h"
#include <execinfo.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include "classifier.h"
#include "led.h"
#include "uart_servo_linux.h"
#include "udp.h"

int *key_scan;         // 获取的键盘状态
int key_scan_gpio = 0; // 获取按键状态

int fd_fb = open_framebuffer();                  // 屏幕设备描述符
int menu_x = 0;                                  // 菜单界面x坐标描述
int menu_y = 1;                                  // 菜单界面y坐标描述
int list_num = 1;                                // 菜单界面列表坐标描述
int list_lenth[9] = {0, 7, 7, 7, 7, 6, 7, 3, 0}; // 各菜单界面列表长度
int start_num = 0;
int base_speed = 2500;
int k = 0;

extern int rows;
extern int cols;
extern uchar *p;
extern Mat frame;
extern Mat grayImage;
extern Mat binaryImage;
extern Mat result;
extern Mat square;
extern Mat g_yuyv;
extern int16 encoder_left;
extern int16 encoder_right;
extern int pwm_servo;
extern int expect_speed;
extern int fast_speed;
extern int circle_save;
extern int barrier_save;
extern uint8_t *PerImg_ip[240][320];



void cleanup()
{
    pwm_clean();
    cout << "PWM cleaned up." << endl;
    led_clean();
    cout << "LEDs cleaned up." << endl;
    close_video();
    cout << "Video closed." << endl;
    pit_ms_clean();
    cout << "PIT timer deleted." << endl;
    close_framebuffer(fd_fb);
    cout << "Framebuffer closed." << endl;
    system("dmesg -n 7");
    cout << "System messages cleared." << endl;
    camera_mmap_clean();
    cout << "Camera memory-mapped buffers cleaned up." << endl;
    model_unload();
    cout << "Model unloaded." << endl;
}

void signal_handler(int signum) {
    // 捕获 SIGSEGV 或 SIGBUS
    if (signum == SIGSEGV || signum == SIGBUS) {
        /* if (signum == SIGSEGV) {
            printf("\n[Error] Segmentation fault (SIGSEGV) detected!\n");
        } else {
            printf("\n[Error] Bus error (SIGBUS) detected!\n");
        }
        
        // 用于存储堆栈地址的数组
        void *array[10];
        // 获取当前堆栈的层数
        size_t size = backtrace(array, 10);

        printf("--- Stack Trace (Call History) ---\n");
        // 将堆栈中的地址转换为具体的函数名并打印到终端
        // 注意：编译时请务必加上 -rdynamic 参数，否则只能看到地址
        backtrace_symbols_fd(array, size, STDOUT_FILENO);
        printf("----------------------------------\n");
        
        // 发生内存错误时退出 */
        exit(EXIT_FAILURE);

    } else if (signum == SIGINT) {
        printf("\n[Info] Interrupt signal (Ctrl+C) received.\n");
        exit(EXIT_SUCCESS);
    }
}

int getch()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // 设置非阻塞读取，这样循环不会卡在等待输入上
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    return ch;
}

int main()
{

    start_load(3);
    start_num = 3;

    /* // gpio初始化
    if(servo_init("/dev/ttyS0", 1) != 0) {
        cerr << "[Error] Failed to initialize servo." << endl;
        return 0;
    }
    servo_init("/dev/ttyS0", 1);
    float current_angle = 0.0f;
    const int SERVO_ID = 1; // 请根据实际情况修改舵机ID
    string input_str;

    // 初始显示一次状态
    system("clear");
    cout << "Current Angle: " << current_angle << " deg" << endl;
    cout << "Battery Voltage: " << get_battery_voltage() << endl;
    cout << "Enter Angle or 'q' to quit: ";

    while (cin >> input_str) {
        // 1. 检查是否输入 'q' 或 'Q'
        if (input_str == "q" || input_str == "Q") {
            cout << "Exiting..." << endl;
            break;
        }

        // 2. 尝试解析输入为浮点数 (Angle)
        try {
            size_t pos;
            float val = stof(input_str, &pos);

            // 确保整个字符串都是有效的数字
            if (pos != input_str.length()) {
                throw invalid_argument("Invalid format");
            }

            // 更新角度并调用舵机控制函数 (时间固定为 0 ms)
            current_angle = val;
            servo_angle_control(SERVO_ID, current_angle, 0);

        } catch (...) {
            cout << "[Error] Invalid input! Please enter a valid number or 'q'." << endl;
            msleep(1000);
        }

        // 3. 刷新显示当前状态
        system("clear");
        cout << "Current Angle: " << current_angle << " deg" << endl;
        cout << "Battery Voltage: " << get_battery_voltage() << endl;
        cout << "Enter Angle or 'q' to quit: ";
    }

    return 0;

 */
    if (CST816_Init() < 0)
    {
        fprintf(stderr, "Failed to initialize `CST816\n");
        return 1;
    }
    printf("\033[?25l"); // 关闭光标显示
    gpio_init(LEFT_WHELL_GPIO, "out", LEFT_FORWARD);
    gpio_init(RIGHT_WHELL_GPIO, "out", RIGHT_FORWARD);
    gpio_init(ENTER_GPIO, "in", 0);
    gpio_init(ESC_GPIO, "in", 0);
    gpio_init(UP_GPIO, "in", 0);
    gpio_init(DOWN_GPIO, "in", 0);
    gpio_init(LEFT_GPIO, "in", 0);
    gpio_init(RIGHT_GPIO, "in", 0);

    led_init();

    pwm_init(LEFT_WHELL_PWM, 25000, 0);
    pwm_init(RIGHT_WHELL_PWM, 25000, 0);
    pwm_init(POWER_TRANSFER, 50, 0);
    pwm_test();

    /* ============ fork 分叉 ============ */
    pid_t child_pid = fork();
    if (child_pid < 0) {
        cerr << "[Fatal] fork() failed" << endl;
        cleanup();
        return 1;
    }
    
    /* ============ 看门狗 ============ */
    if (child_pid > 0) {
        int status;
        while (waitpid(child_pid, &status, 0) == -1) {
            if (errno != EINTR) {
                cerr << "[Watchdog] waitpid error: " << strerror(errno) << endl;
                cleanup();
                return 1;
            }
        }

        if (WIFEXITED(status)) {
            cout << "[Watchdog] Child exited normally, code="
                 << WEXITSTATUS(status) << endl;
        } else if (WIFSIGNALED(status)) {
            cout << "[Watchdog] Child killed by signal "
                 << WTERMSIG(status) << endl;
        }

        cleanup();
        return 0;
    }

    /* ============ 子进程 ============ */
    // 注册清理函数
    atexit(cleanup);

    // 注册信号处理函数
    signal(SIGINT,  signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGBUS,  signal_handler);

    /* int power_sum = 500;
    string input_str;

    // 初始显示一次状态
    system("clear");
    cout << "Current PWM: " << power_sum << endl;
    cout << "Battery Voltage: " << get_battery_voltage() << endl;
    cout << "Enter PWM (0-10000) or 'q' to quit: ";

    while (cin >> input_str) {
        // 1. 检查是否输入 'q' 或 "q"
        if (input_str == "q" || input_str == "Q") {
            power_sum = 0;
            pwm_set_duty(POWER_TRANSFER, power_sum); // 清零 PWM 输出
            cout << "Exiting... PWM reset to 0." << endl;
            break;
        }

        // 2. 尝试解析输入为整数并验证范围
        try {
            size_t pos;
            int val = stoi(input_str, &pos);

            // 确保整个字符串都是有效的数字（避免输入 123abc 的情况）
            if (pos != input_str.length()) {
                throw invalid_argument("Invalid format");
            }

            // 验证输入是否在 0 - 10000 范围
            if (val >= 0 && val <= 10000) {
                power_sum = val;
                pwm_set_duty(POWER_TRANSFER, power_sum);
            } else {
                cout << "[Error] Input out of range! Please enter a value between 0 and 10000." << endl;
                msleep(1000);
            }
        } catch (...) {
            cout << "[Error] Invalid input! Please enter an integer or 'q'." << endl;
            msleep(1000);
        }

        // 3. 刷新显示当前状态
        system("clear");
        cout << "Current PWM: " << power_sum << endl;
        cout << "Battery Voltage: " << get_battery_voltage() << endl;
        cout << "Enter PWM (0-10000) or 'q' to quit: ";
    }

    return 0;  */

    /* while(1){
        encoder_left = encoder_get_count(ENCODER_LEFT);
        encoder_right = -encoder_get_count(ENCODER_RIGHT);
        system("clear");
        cout << encoder_left << " " << encoder_right << endl;
        msleep(100);

    } */
    while (keyboard_scan_cst816() == -1)
    {
    };
    camera_mmap_init();
    //camera_mmap_perf_test();
    // get_camera_message();
    get_camera_mmap_message();
    if (!model_load("75ttv1"))
    {
        return 0;
    }
    while (keyboard_scan_cst816() == -1)
    {
    };
    
    //test_images_in_directory("/home/root/image/");
    play_video("/home/root/Miku.gif", 10, 199, 0); 
    // test_uart();
    while (1)
    {

        key_scan_gpio = keyboard_scan_cst816();
        int key_scan_gpio2 = keyboard_scan_gpio();

        if (key_scan_gpio == -1 && key_scan_gpio2 == -1)
        {

            continue;
        }

        switch (key_scan_gpio)
        {

        case ENTER:

            if (menu_x == 0)
            {

                switch (list_num)
                {

                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:

                    menu_x++;
                    menu_y = list_num;
                    list_num = 1;
                    break;

                case 7:

                    exit(0);
                    return 0;

                default:

                    break;
                }
            }
            else if (menu_x == 1 && menu_y == 1)
            {

                setting1(list_num, start_num);
            }
            else if (menu_x == 1 && menu_y == 2)
            {

                setting2(list_num, start_num);
            }
            else if (menu_x == 1 && menu_y == 3)
            {
                setting3(list_num, start_num);
            }
            else if (menu_x == 1 && menu_y == 4)
            {

                char path[100];
                close_video();
                while (1)
                {

                    key_scan_gpio = keyboard_scan_gpio();
                    if (key_scan_gpio == DOWN)
                    {

                        key_scan_gpio = 0;

                        break;
                    }
                    if (key_scan_gpio == ENTER)
                    {

                        key_scan_gpio = 0;
                        system("clear");
                        uint64_t start_time = get_wall_time_ns();
                        snprintf(path, sizeof(path), "/home/root/image/image_%d.png", k);
                        k++;
                        Mat g_yuyv2bgr;
                        cvtColor(g_yuyv, g_yuyv2bgr, COLOR_YUV2BGR_YUYV);
                        imwrite(path, g_yuyv2bgr);
                        uint64_t end_time = get_wall_time_ns();
                        uint64_t elapsed_time = end_time - start_time;
                        // cout << imwrite(path, frame) << endl;
                        cout << "time: " << elapsed_time / 1e6 << " ms" << endl;
                        usleep(500000);
                    }
                    if (key_scan_gpio == LEFT)
                    {

                        key_scan_gpio = 0;
                        system("clear");
                        // cout << find_barrier() << endl;
                        HSV hsv = yuyv_to_hsv(200, 159); // 查询第119行、第159列的HSV
                        cout << "H= " << hsv.h << endl;
                        cout << "S= " << hsv.s << endl;
                        cout << "V= " << hsv.v << endl;
                        msleep(1000);
                    }
                    if (list_num == 1)
                    {
                        get_mmap_image(1);
                        print_image_toushi(0);
                    }
                    else if (list_num == 2)
                    {
                        get_mmap_image(2);
                        print_image(grayImage,0,0);
                    }
                    else if (list_num == 3)
                    {
                        get_mmap_image(3);
                        print_image_toushi(1);
                        for (int i = 0; i < 240; i++)
                        {

                            print_pixel(i, 159, "black");
                        }
                        for (int i = 0; i < 320; i++)
                        {

                            print_pixel(200, i, "black");
                        }
                        usleep(100000);
                    }
                    else if (list_num == 4){
                        get_mmap_image(3);
                        print_image(g_yuyv, 0, 0);
                        for (int i = 0; i < 240; i++)
                        {

                            print_pixel(i, 159, "black");
                        }
                        for (int i = 0; i < 320; i++)
                        {

                            print_pixel(119, i, "black");
                        }
                        usleep(100000);
                    }
                    else if(list_num == 5){
                        get_mmap_image(4);
                        print_image_toushi(0);
                    }
                    else if(list_num == 6){
                        get_mmap_image(5);
                        print_image(grayImage,0,0);
                    }
                }
                play_video("/home/root/Miku.gif", 10, 199, 0); 
            }
            else if (menu_x == 1 && menu_y == 5)
            {
                close_video();
                switch (list_num)
                {
                case 1:

                    start_save(start_num);
                    break;

                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                    start_num = list_num;
                    start_load(start_num);
                    main_run();

                    break;
                default:

                    break;
                }
                led_clean();
                play_video("/home/root/Miku.gif", 10, 199, 0); 
            }
            else if (menu_x == 1 && menu_y == 6){
                close_video();
                switch (list_num)
                {
                case 1:
                    web_service_menu();
                    break;
                case 2:
                    camera_setting();
                    break;
                case 3:
                    test_udp("10.28.65.75", 9000);
                    break;
                default:
                    break;
                }
                play_video("/home/root/Miku.gif", 10, 199, 0);

            }
            break;

        case ESC:

            if (menu_x >= 1)
            {

                menu_x--;
                menu_y = 1;
                list_num = 1;
            }
            break;

        case UP:

            if (list_num > 1)
            {

                list_num--;
            }
            else
            {

                list_num = list_lenth[menu_x + menu_y];
            }
            break;

        case DOWN:

            if (list_num < list_lenth[menu_x + menu_y])
            {

                list_num++;
            }
            else
            {

                list_num = 1;
            }
            break;

        default:

            break;
        }
        if(key_scan_gpio2 == ENTER && menu_x == 1 && menu_y == 5){
            close_video();
            start_num = 7;
            start_load(7);
            main_run();
            led_clean();
            play_video("/home/root/Miku.gif", 10, 199, 0);
        }

        menu_display(menu_x, menu_y, list_num);
        usleep(200000);
    }

    return 0;
}

/*
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣠⣤⣤⣤⣤⣤⣤⣤⣤⣤⣄⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣤⠶⠞⠛⠉⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠛⠳⠶⣤⣀⠀⠀⠀⠀⣾⣟⠻⡙⢯⢿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⢿⡷⠶⣤⣤⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀⣀⣤⠶⠞⠋⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⢦⣄⠀⢻⣿⡄⢱⡈⢏⢿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣦⡀⠀⠉⠉⠙⠛⠛⠛⠛⠛⠛⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢳⣿⣿⣿⡄⢣⠘⡎⢿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⢦⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⡄⢇⠸⡜⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡏⡏⢨⡗⣦⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠢⡀⠀⠀⠀⠀⠙⣿⣷⡈⢇⠸⡜⣷⡶⠿⢷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⢳⠃⣿⠽⠋⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣄⠀⠀⠀⠀⠈⢿⣷⠈⣆⠱⠹⡆⢀⠈⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣶⠛⠛⣟⡾⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⠙⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡄⠈⢧⡄⠀⠀⠀⠈⢿⣇⠘⡄⠳⢻⡀⢀⠹⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡟⣠⠜⠃⠀⠀⠀⠀⠀⠀⢀⠎⠀⠀⠀⠀⠀⠀⠀⠀⡆⢳⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⠈⢦⠀⠀⠀⠀⠀⠀⠀⡠⠖⠉⠀⠀⠀⢻⡦⠀⠀⠛⠒⠿⡇⢸⡀⢏⢷⠀⡀⢿⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⠗⠁⠀⠀⠀⢀⣠⡖⠀⠀⡎⠀⠀⢀⡀⠀⠀⠀⠀⡼⠀⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢈⡀⡄⠀⢷⡀⠀⠀⣠⠔⠋⠀⠀⠀⠀⠀⠀⡀⢳⡱⡀⢀⠠⠀⣯⣥⡇⣼⣾⡇⠀⠈⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡾⠁⠀⠀⢀⣠⡖⢿⠏⠀⠀⣰⠁⠀⠀⠀⠉⠲⢄⡀⣴⢇⠃⠨⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡇⡇⠀⠀⢻⣴⠊⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⢷⡈⠂⠠⡴⣿⣿⡇⣻⣽⠁⢀⠁⢸⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⠟⠀⢀⡤⠖⣹⣿⣇⠏⠀⠀⡔⡏⠀⠀⠀⠀⠀⠀⠀⢉⡾⢄⡀⠀⢹⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡇⠁⣠⠔⠁⢹⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⢸⣧⠀⠙⠁⣿⣿⠃⣿⣿⠀⠀⠌⠀⢿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⠏⣀⣴⣿⠀⢀⢿⣿⡏⠀⠀⡔⢸⡇⠀⠀⠀⠀⠀⠀⡠⢹⠁⠀⠈⠑⠬⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣷⠈⠁⠀⠀⠀⠘⣦⠀⠀⠀⠀⠀⠀⠀⠀⢸⣀⡸⣿⣆⡐⠀⣿⣿⠀⣿⡧⠀⠌⠀⠂⠘⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⣯⡾⠋⢠⡟⠀⢸⠈⣿⠀⠀⡝⣒⢼⢁⠀⠀⠀⠀⠀⢠⢃⡏⠁⠀⠀⠀⠀⠘⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⠀⠀⠀⠀⠀⠈⢗⠀⠀⠀⠀⠀⠀⠀⢠⠀⠒⣿⣷⡀⢡⣿⣿⠀⣿⡇⠀⢂⠈⠄⠁⢹⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠁⠀⠀⣾⠁⢀⠇⢠⡇⠀⢠⠃⠀⣹⢰⠀⠈⠀⠐⠂⠾⢸⠁⠀⠀⠀⠀⠀⠀⠹⡀⠂⠀⠀⠀⠀⠀⠀⠀⢸⡆⠀⠀⠀⠀⠀⠀⠀⢳⣠⡶⠀⠀⠀⠀⠘⢀⠀⣿⣯⣇⠰⣿⣽⢀⣿⠀⠐⡀⢀⠂⢈⠀⢿⣆⠀⠀⢀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⠏⠀⡼⠀⢸⠁⢀⡇⠀⡁⣿⠈⡇⠀⠀⠀⢸⠉⡟⠀⠀⠀⠀⠀⠀⠀⠀⠹⡄⠀⠀⠀⠀⠀⠀⠀⡄⡇⠀⠀⠀⠀⠀⣠⡴⠟⢻⡀⠀⠀⠀⠀⡜⠀⡀⣿⣿⣹⢐⣿⡏⢸⣿⠀⠐⠀⢠⣰⡤⡞⠛⣿⣶⠞⠛⣹⣇⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⡟⠀⢠⠃⠀⣻⠀⣸⠀⢠⠀⢻⠀⢰⠀⠀⢀⡇⢸⠳⢦⣄⡀⠀⠀⠀⠀⠀⠀⠹⡄⠀⠸⡄⠀⠀⠀⡇⢹⠀⠀⢀⣤⠾⠋⠀⠀⠀⢷⡀⠀⠀⠀⡇⠀⡀⣿⣿⡍⣳⣿⣧⣼⣿⣤⠶⠛⠉⠁⠀⡇⠀⢹⠁⠀⢸⠙⠉⣀⣽⠗⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡴⠶⠶⣿⡉⠙⠺⣄⢠⡇⢠⠇⢀⣿⡃⢸⡀⠌⡇⠀⣸⠀⣼⠀⠀⠈⠙⠳⢦⣄⣀⠀⠀⠀⠘⢆⠀⠹⢄⠀⠀⢧⢸⣤⣶⠛⠁⠀⠀⠀⠀⠀⠈⢧⣀⠀⠀⠇⠀⠄⣿⠋⡇⣏⣿⠿⠛⠁⠀⠀⠀⠀⠰⠀⣷⠀⠘⡆⠁⠁⢸⢹⠙⢻⡦⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢈⣷⡶⠼⡄⠉⢶⠀⠈⡽⡇⠸⠀⣾⣿⡇⠘⣇⠀⢱⢀⡇⠀⡇⠀⠀⠀⠀⠀⠀⠈⣩⡿⠆⠀⠀⠈⠳⣈⣆⠳⡀⢸⠀⡏⠉⠛⠷⠶⣤⣀⡀⠀⠀⠈⣇⢆⢰⠀⢈⢰⣷⣠⠟⠋⠀⠀⠀⠀⡀⠄⠐⠈⠐⠀⢹⠀⠀⢣⠀⠀⣽⣈⠛⢿⡅⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⣥⣤⠤⣇⠀⠈⠣⢰⠁⢹⠀⣰⣿⡏⣷⠀⢻⠀⠌⠊⢀⠐⡇⠀⠀⠀⠀⣀⣴⠞⠋⠀⠀⠀⠀⠀⠀⠈⠛⢦⡙⠌⠀⢸⠀⠀⠀⠀⠀⠉⠙⠳⢶⡄⡟⣎⠘⠀⠂⢸⣿⡿⠀⢀⠀⠠⠀⠀⠀⠀⠀⠀⢀⠠⢸⡄⠀⠘⣆⣠⢥⣉⣿⠛⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣾⢻⡡⣤⠟⠆⠀⢀⠏⠀⡾⡇⣿⡟⠀⠸⣇⢸⣧⠀⠌⠀⢸⠁⠀⣠⣴⠟⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠓⢴⣸⣤⠤⡀⠀⠀⡔⠐⡠⠰⠀⡇⢹⡄⠈⠐⣾⣿⠃⠀⠀⠀⢀⠀⠈⠀⡀⠁⠀⠀⠀⠐⡇⠀⠀⢳⠈⣆⠻⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡟⠁⠀⣀⠓⠒⠟⠒⡾⠀⢰⡇⠿⠟⠀⠀⠀⠹⣼⡿⣆⠠⠁⢸⠀⠈⠉⡀⠀⠀⠀⠀⠀⠀⠀⠀⢠⠤⡤⢤⠤⠤⣒⢒⡫⠭⣃⣆⡙⢦⠀⠈⠉⠁⠁⠀⡇⡄⢷⠈⢰⣿⡇⠀⠀⠀⠂⠀⠀⠐⠀⠀⠀⠐⠈⠀⠀⢿⠀⠀⢸⡆⠘⡄⠹⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡾⠋⠀⢀⡔⠁⠠⠀⠂⢨⡇⠀⣸⢀⠀⠀⠀⠀⠄⠀⠙⠀⠹⣆⠀⣽⡆⠀⢡⢁⠢⠃⠀⠀⠀⠀⠀⠀⣏⡷⠖⠊⠉⠁⠀⠀⠀⠀⠀⠀⠈⠑⢻⠀⠀⠀⠀⠀⡇⡇⣸⡇⣼⡿⠀⠀⠀⠁⠀⠀⠂⠀⠂⠀⠁⠀⠀⠐⠀⢸⡀⠀⠈⡇⠀⢳⠀⠻⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣴⠟⠁⠀⢀⠎⠀⠄⠁⠠⠁⢸⠀⠀⡟⢊⠀⠀⠄⠂⠀⡀⠂⠀⠀⢻⣆⣾⢧⠀⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢧⠀⠀⣀⣴⡟⣠⣿⣿⣿⠁⠀⠀⠁⠀⠂⠁⠀⠠⠀⠂⠀⠐⠈⠀⣀⣘⣇⣀⣀⡧⠐⠀⢧⠀⠻⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡾⠃⠀⠀⣠⠋⠀⠌⠀⠌⠀⠂⣿⠀⢀⡇⠀⠀⠠⠀⠀⠄⠀⠀⠈⠀⠈⣏⢿⢸⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡼⠔⢺⡁⢸⣧⣿⣿⡿⠇⠀⠀⠐⠀⠐⣀⣠⣤⣤⣴⡶⠿⠿⣯⠉⠉⠉⢁⠠⠀⠄⠐⠈⠘⡄⠀⢻⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⠏⠀⠀⠀⡴⠁⢀⠂⠐⠈⡀⠌⠀⣿⠀⢸⠃⠀⠀⠂⠀⠄⠠⠀⠂⠀⡁⠀⠹⡄⢺⣿⣿⣷⣶⣤⣄⣀⣀⠀⠀⠀⠸⡀⠀⠀⠀⠀⠀⣀⣀⣤⠤⠒⢺⠋⢁⠠⠀⠈⡇⢸⢿⣿⡟⣀⣠⣤⣶⡶⠿⠛⠛⠉⠉⠀⠀⠀⠀⠀⢹⣇⠈⠄⠂⢀⠂⢈⠠⠈⢀⠹⡀⠀⢻⣦⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⠁⠀⠀⠀⣸⠁⠀⡀⠰⠈⠀⡀⠰⢀⠉⠉⠿⢿⣿⣷⣶⣶⣶⣶⣶⣆⣀⣰⣀⡀⢿⣸⣿⡇⢉⠿⠿⢿⣿⡇⠉⡿⠉⠉⠉⠉⠉⡿⠉⢏⠀⢰⠇⠀⠀⣾⢰⠀⠀⢀⣀⣾⣀⣾⣿⠿⠏⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣀⠰⠈⠀⠀⠆⠀⠰⠀⠀⢇⠀⠀⢿⡆⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣴⠟⠁⠀⠀⠀⡰⠃⠀⡐⠀⡐⠀⡁⠠⠐⠀⡀⠂⢀⣾⠃⠀⠀⠀⠀⠀⠉⠉⠉⠉⠙⠛⠛⢿⣿⠛⠛⠿⠶⠶⣾⣅⣹⡷⠮⣤⣤⣄⡴⢻⢀⣨⣧⣯⡀⠀⡸⠘⣄⣤⠾⠛⠉⠉⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣇⠀⡐⠈⢀⠂⠁⠄⢁⠈⠸⡄⠀⠈⢿⡀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡾⠃⠀⠀⠀⠀⢰⠃⠀⢂⠀⡐⠀⡐⠀⡐⠀⡁⠠⢈⣾⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠁⠀⠀⠀⠀⠀⠀⠈⠉⠀⠀⠀⠀⣿⠁⠘⢺⠛⠐⠀⢸⠳⠁⠀⢿⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⡀⠠⠐⠀⠠⠈⡀⠄⠂⡀⢣⠀⠀⠈⣷⡀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⣴⠟⠀⠀⠀⠀⠀⢠⠇⠀⠌⠀⠠⢀⠐⠀⡐⠀⡐⠀⢤⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡇⠀⠀⢈⣦⠴⢶⡎⠀⠀⠀⢸⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣇⠀⠄⠁⠂⠁⡀⠄⠂⢀⠘⡆⠀⠀⠘⣷⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⢀⡾⠃⠀⠀⠀⠀⠀⢀⠏⠀⡐⠀⠡⠐⠀⡀⠂⠠⠐⠀⢠⡿⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡟⠀⠀⡰⠋⠀⠁⢺⠃⠀⠀⠀⠘⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⠐⠈⡀⢁⠠⠐⠈⢀⠠⢱⠀⠀⠀⠹⣇⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⢠⡿⠁⠀⠀⠀⠀⠀⠀⡜⠀⡐⠀⡈⠄⠠⠁⠠⢀⠁⡐⢨⡿⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⣾⠛⠁⢀⡞⠁⠀⠀⠀⠈⡇⠀⠀⠀⠀⢿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⡆⠈⠀⠄⠠⠀⢂⠈⠀⠄⠈⡇⠀⠀⠀⢻⡆⠀⠀⠀
⠀⠀⠀⠀⠀⣰⡟⠀⠀⠀⠀⠀⠀⠀⢰⠃⠠⠀⢂⠀⡐⠀⠂⢁⠠⠀⢠⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣶⣶⣦⣤⣴⠞⠛⠁⠀⠀⠀⡎⠀⠀⠀⠀⠀⢀⡇⠀⠀⠀⠀⠺⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡇⢀⠡⠐⠀⡁⠠⠀⠡⠀⠁⢣⠀⠀⠀⠀⣿⡀⠀⠀
⠀⠀⠀⠀⢰⡟⠀⠀⠀⡴⠀⠀⠀⠀⡜⠀⠄⠁⠠⠀⠄⠂⢁⠠⠀⢂⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⣿⣟⣿⡄⣿⣿⠀⠀⠀⠀⠀⠀⠸⡄⠀⠀⢀⡤⠊⠀⠀⠈⠀⠀⠀⠻⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡇⠠⠀⠂⢁⠠⠀⡁⠐⠈⠐⠸⡄⠀⠀⠀⠘⣧⠀⠀
⠀⠀⠀⢠⡿⠀⠀⢀⣼⠇⠀⠀⠀⢠⠁⠐⠈⡀⠁⠄⠂⢈⠀⠄⠂⣼⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⡾⣽⣣⡿⣿⡄⠀⠀⠀⠀⠀⠀⢹⡠⠔⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡇⢀⠂⠁⠠⠀⢂⠀⠌⢀⠡⠀⠃⠀⠀⠀⠀⢻⡄⠀
⠀⠀⢀⡿⠀⠀⢀⣾⡿⠀⠀⠀⠀⢸⠀⠈⠄⠐⠈⡀⠐⡀⠄⠂⢰⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⣿⣻⣿⡟⣽⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⣿⣶⣶⣷⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⠃⢀⠠⠁⠠⠁⠠⠀⢂⠠⠀⢂⠀⠀⠀⠀⠀⠘⣧⠀
⠀⠀⣼⠃⠀⢀⡾⢹⡇⠀⠀⠀⠀⡇⠀⠌⠀⠂⢁⠠⠐⠀⡀⠂⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⢿⣯⡟⢰⣿⣇⠀⣀⣠⣴⣶⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⡿⣿⣿⣿⣻⡏⢸⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡿⡀⠠⠀⠂⢁⠠⠁⠐⡀⢀⠂⠠⠀⠀⠀⠀⠀⠀⢻⡄
⠀⢰⡏⠀⢀⣾⠃⢸⡇⠀⠀⠀⠀⠃⢀⠂⠁⡈⠀⠄⠠⡁⢀⢸⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣤⡾⠟⢻⣈⣧⠼⣯⡟⠁⠀⠀⠈⠙⣦⡀⠀⠀⠀⠀⠀⢀⡴⠛⣿⣆⠈⢻⡿⣩⣿⣼⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⠇⠀⠄⠁⡐⠀⡀⠂⢁⠠⠀⠄⠁⡀⠀⠀⠀⠀⠀⢸⡇
⠀⣼⠁⠀⣼⠃⠀⢾⠀⠀⠀⠀⢀⠀⠂⢀⠂⢀⠡⠐⢸⡇⠀⣼⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣤⡶⠟⠋⠉⢧⡀⡀⠻⠟⢳⡀⠳⣄⣠⣄⣤⣀⣿⠘⠢⣄⢀⣠⢶⡿⠖⣛⣛⣻⡴⠋⣸⡟⠿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⡟⠀⣨⠀⢂⠠⠐⠀⡐⠀⡀⠂⢈⠀⠄⠀⠀⣦⠀⠀⠀⣿
⢠⡟⠀⢰⡏⠀⠀⣻⡀⠀⠀⠀⢸⠀⢈⠀⠄⠂⠀⠄⣿⣇⠀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣤⣴⠶⠟⠛⠉⠁⠀⠀⠀⠀⠈⢳⣈⣠⡶⠟⠻⣦⣀⣤⣤⣤⣬⣟⡀⠀⢈⣩⡽⢾⣶⡟⠛⢹⣿⣡⡴⠟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⠁⢰⣿⠀⠄⢀⠂⠁⠠⠐⠀⡈⠀⠄⠈⠀⠀⣿⠀⠀⠀⣿
⢸⡇⢀⡿⠀⠀⠀⢹⡇⠀⠀⠀⢸⠀⠠⠀⢂⠈⠐⢀⣿⣿⠠⣿⠀⠀⠀⠀⠀⠀⠀⠀⣠⣴⣿⣿⠛⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠐⣠⣵⡾⠛⠁⠀⠀⠀⠀⠀⠀⠀⠸⢿⣌⡙⠛⢉⣠⠴⠚⣿⠁⠀⠀⠈⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⠇⢰⣿⡟⠀⡀⠂⠠⠈⡀⠄⠁⠠⠈⠀⡆⠀⢰⣿⠀⠀⠀⣿
⢸⠅⣸⠃⠀⠀⠀⠸⣇⠀⠀⠀⠘⡀⠄⠁⠠⠀⡁⢸⡏⢿⡴⣿⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⢯⣿⡆⠀⠀⠀⠀⢀⠠⠀⢂⣡⣼⠿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣻⠏⠉⠀⢀⠠⢿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⡏⢠⡿⣹⡇⢀⠐⠀⡁⠄⠠⠐⠈⡀⠂⠁⡆⠀⣾⢿⠀⠀⠀⣿
⢸⡇⡿⠀⠀⠀⠀⠀⢿⡀⠀⠀⠀⡇⠀⠌⢀⠐⠀⣼⡇⠘⣷⣿⠀⠀⠀⠀⠀⠀⠀⠀⠘⢿⣿⣞⣿⣄⠀⠀⣠⣤⣴⠾⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⢄⣀⣀⣀⡠⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡿⢠⡿⠁⣿⠀⠠⠀⡁⠠⠐⠀⠂⢁⠠⠀⡁⠇⣼⠃⢸⡄⠀⢠⡟
⢸⣧⡇⠀⠀⠀⠀⠀⠸⣧⠀⠀⠀⢧⠐⠀⠂⡀⠂⣿⠇⠀⢻⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠛⠿⢿⣿⡿⠿⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⡀⠀⠀⢀⠰⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣾⢣⣿⠁⠀⣿⠀⠁⠄⠐⠀⠄⢁⠈⡀⠄⠐⢸⣾⠋⠀⢸⡇⠀⢸⡇
⠀⢻⠇⠀⠀⠀⠀⠀⠀⠹⣇⠀⠀⢸⠀⠈⠄⠀⠄⣿⠀⠀⠀⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡇⡇⠀⢀⠀⢸⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⢇⣿⠃⠀⠀⣿⠀⢈⠠⠈⠀⠌⠀⠠⢀⠐⣠⡿⠃⠀⠀⢸⠀⠀⣿⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣦⠀⠀⡆⠁⠠⠈⠀⣿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⢇⠁⠀⠠⠀⢸⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣟⣾⠃⠀⠀⢰⡿⠀⠀⠄⠂⢁⠠⠈⡀⢄⣼⠟⠀⠀⠀⠀⣿⠀⢸⡇⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢷⡀⢹⠀⠁⠠⠁⢿⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠾⠀⠀⠀⠄⢺⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⣻⠏⠀⠀⠀⢸⡇⠠⠁⠠⠈⠀⠄⠐⣠⡿⠃⠀⠀⠀⠀⠀⣿⢀⡿⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠻⣮⣦⠈⢀⠂⢸⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡏⡐⠀⠠⠐⠀⣿⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣿⡟⠀⠀⠀⠀⢸⡇⠀⠂⢁⠠⠁⢠⣾⠏⠀⠀⠀⠀⠀⠀⢠⡏⣾⠃⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠙⢿⣴⡀⠈⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⠛⠁⠀⠀⡀⢀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⣿⠁⠀⠀⠀⠀⢸⡇⠀⠡⠀⣀⣶⠟⠁⠀⠀⠀⠀⠀⠀⠀⣸⣿⠇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠛⢷⣿⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣟⠂⠀⠄⠁⠀⢸⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠃⠀⠀⠀⠀⠀⢸⡇⠈⡀⣰⡾⠋⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⠟⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣷⣦⣤⣤⣶⣾⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢺⡇⢀⣼⠟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢾⠏⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⢿⣷⣯⣟⣿⣿⠟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣷⡟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠙⠛⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
*/
