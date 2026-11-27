#ifndef _MENU_H_
#define _MENU_H_

#include "zf_common_headfile.hpp"

/****************************** Screen Config ******************************/
#define  SCREEN_WIDTH           (240) // 屏幕宽度
#define  SCREEN_HEIGHT          (320) // 屏幕高度
#define  SCREEN_FONT_SIZE       (18)  // 屏幕字体大小 显示为18，实际为16，防止菜单界面过于紧凑
/****************************** Menu Config ******************************/
#define  MENU_MAX_SIZE          (64) // 最大菜单数量
#define  MENU_NAME_MAX_SIZE     (16) // 菜单名称最大长度
#define  STEP_LENGTH            (7)  // 设置步进值数量
#define  STEP_DEFAULT_NUMBER    (3)  // 设置步进值基础值
/****************************** Button Config ******************************/
#define  KEY_1_PATH             ZF_GPIO_KEY_4 // 上键
#define  KEY_2_PATH             ZF_GPIO_KEY_1 // 下键
#define  KEY_3_PATH             ZF_GPIO_KEY_5 // 确认键
#define  KEY_4_PATH             ZF_GPIO_KEY_2 // 进入键
#define  KEY_5_PATH             ZF_GPIO_KEY_3 // 退出键
#define  KEY_WAIT_MS            (10)
#define  KEY_MAX_NUMBER         (5)
/****************************** UI Config ******************************/
#define  USE_UI_MENU            (0) // 是否使用UI菜单系统

/****************************** Menu Config ******************************/
enum menu_type // 菜单节点类型
{
    menu_folder = 0,
    int8_data,                  // int8_t
    int16_data,                 // int16_t
    int32_data,                 // int32_t
    uint8_data,                 // uint8_t
    uint16_data,                // uint16_t
    uint32_data,                // uint32_t
    float_data,                 // float
    bool_data,                  // bool
};
struct UI_menu // 菜单节点结构体
{
    const char *name;           // menu name
    void *data;                 // 指向存放的变量
    menu_type type;             // 记录节点属性
    uint8_t member;             // 记录子节点数量
    uint8_t sequence_number;    // 记录当前是父节点的第几个成员
    bool select;                // 是否被选中
    struct UI_menu *totalnode;  // 父节点
    struct UI_menu *subnode;    // 子节点
    struct UI_menu *nextnode;   // 下一个兄弟节点
    struct UI_menu *lastnode;   // 上一个兄弟节点
};
extern const char* menu_action_name; // 菜单动作名称
/****************************** Button Config ******************************/
extern uint8_t key_num; // 按键数量
extern zf_driver_gpio key_1, key_2, key_3, key_4, key_5; // 按键引脚

/****************************** Menu Config ******************************/
void UI_menu_init(void); // 初始化菜单系统
void UI_menu_display(void); // 显示菜单系统
void creat_UI_menu(UI_menu *totalnode, UI_menu *nownode, const char name[], void *data, menu_type type); // 创建菜单节点
UI_menu* creat_UI_menu_folder(UI_menu *totalnode, const char name[]); // 创建文件夹节点
void creat_UI_menu_parameter(UI_menu *totalnode, const char name[], void *data, menu_type type); // 创建参数节点
void link_data_init(UI_menu *arr); // 初始化菜单节点数据，包括子节点数量、记录当前是父节点的第几个成员、是否被选中
void display_title(void); // 显示菜单标题
void display_menu(void); // 显示菜单
void arrow_scan(void); // 扫描方向键
/****************************** Button Config ******************************/
void key_select(void); // 扫描自锁键
void key_increase(void); // 数据增加
void key_decrease(void); // 数据减少
uint8_t key_scan(void); // 扫描按键

#endif
