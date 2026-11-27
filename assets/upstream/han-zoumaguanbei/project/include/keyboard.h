#ifndef KEYBOARD_H
#define KEYBOARD_H

// 键码
#define UP                 103     // 上
#define DOWN               108     // 下
#define LEFT               105     // 左
#define RIGHT              106     // 右
#define ENTER              28      // 确认
#define ESC                1       // 退出

// 获取按键状态及键值
int* keyboard_scan(int fd_keyboard);

// 获取GPIO按键状态
int keyboard_scan_gpio();

void cst816_test();

int keyboard_scan_cst816();

#endif // KEYBOARD_H