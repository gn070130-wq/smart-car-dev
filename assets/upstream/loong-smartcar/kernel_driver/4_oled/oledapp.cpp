#include <cstdio>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "oled.h"

int main() {
    int fd = open("/dev/oled", O_RDWR);
    if (fd < 0) {
        perror("open device failed");
        return -1;
    }

    // 显示字符'A'在(0,0)，16号字体，正常模式
    struct oled_char_param chr = {0, 0, (unsigned char*)"ABCDE", 16, 1};
    ioctl(fd, OLED_SHOW_CHAR, &chr);

    // 显示数字123在(0,16)，长度3，16号字体
    struct oled_num_param num = {0, 16, 123, 3, 16, 1};
    ioctl(fd, OLED_SHOW_NUM, &num);

    while (1) {
        // 无限循环，防止程序退出
    }

    // 清屏
    ioctl(fd, OLED_CLEAR);

    close(fd);
    return 0;
}