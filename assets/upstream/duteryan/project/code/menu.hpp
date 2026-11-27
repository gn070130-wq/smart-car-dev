#ifndef __MENU_HPP
#define __MENU_HPP

#include "zf_common_headfile.hpp"

// 小车运行状态标志
extern uint8_t car_running;       // 1=小车运行    0=小车停止
extern std::atomic<uint8_t> launch_request;//首页长按发车后置1，由主线程完成全状态清理再启动
extern uint8_t ui_show_enable;    // 1=允许显示菜单 0=关闭显示
extern int im_dis_type;//显示图像类型，0---灰度图像，1---二值化图像
extern int camera_exposure_mode;//曝光模式：0=自动，1=手动
extern int camera_manual_exposure;//手动曝光值，默认140，菜单每次±5
// 字体行高 20px（固定）
#define FONT_HEIGHT       20

// 菜单页面枚举（定义所有菜单页面编号）
typedef enum {
    PAGE_HOME        = 0,        // 首页：显示图像 + 实时数据 + 右上角菜单
    PAGE_PARA1       = 1,        // 参数页1
    PAGE_PARA2       = 2,        // 参数页2
    PAGE_PARA3       = 3,        // 参数页3
    PAGE_PARA4       = 4,        // 参数页4
    PAGE_PARA5       = 5,        // 参数页5
    PAGE_CAMA1       = 10        // 相机参数页
} MenuPage;

// 整数参数显示结构体（和你的存储结构体完全匹配）
typedef struct {
    const char *label;           // 参数名称
    int        *value;           // 参数变量指针
} DisplayItem_int;

// 浮点数参数结构体
typedef struct {
    const char *label;
    float      *value;
} DisplayItem_float;

// 菜单全局变量
extern int  menu_temp_index;     // 当前选中行
extern int  menu_page_num;       // 当前菜单页码
extern int  menu_last_row;       // 上一行（用于清除下划线）

// 实时数据数组
extern DisplayItem_int    real_time_int[];
extern const int          real_time_int_LENGTH;

extern DisplayItem_float  real_time_float[];
extern const int          real_time_float_LENGTH;

// 5页参数 + 相机参数
extern DisplayItem_int Para1[];
extern const int        Para1_LENGTH;

extern DisplayItem_int Para2[];
extern const int        Para2_LENGTH;

extern DisplayItem_int Para3[];
extern const int        Para3_LENGTH;

extern DisplayItem_int Para4[];
extern const int        Para4_LENGTH;

extern DisplayItem_int Para5[];
extern const int        Para5_LENGTH;

extern DisplayItem_int Cama1[];
extern const int        Cama1_LENGTH;

// ===================== 函数声明 =====================
// 菜单主处理函数（按键、页面切换、参数修改）
void menu_isr_handler(void);

// 绘制选择下划线
void menu_draw_underline(int y_pos, uint16 color);

// 绘制矩形框
void draw_rect_box(int x1, int y1, int x2, int y2, uint16 color);

// 绘制首页（右上角菜单）
void menu_draw_home(void);

// 绘制各参数页
void menu_draw_para1(void);
void menu_draw_para2(void);
void menu_draw_para3(void);
void menu_draw_para4(void);
void menu_draw_para5(void);

// 绘制相机参数页
void menu_draw_camera(void);

// 显示灰度图像 + 实时数据 + 标线 + 粉色标尺
void ShowGrayImgParams(const uint8_t *gray_img, uint8_t thresh);

// 显示RGB图像
void ShowRGBImgParams(const uint16_t *rgb_img);

// 启动小车运行
void start_standalone_task(void);
void prepare_new_run(void);//主线程调用：清空上次运行状态后再置car_running=1
// 封装函数：屏幕自动显示 / 黑屏管理（无输入参数）
void screen_auto_display(void);
// 摄像头重新初始化
void camera_reinit(void);
void sync_camera_exposure_config(void);//把菜单0/1模式转换为UVC底层1/3模式

#endif
