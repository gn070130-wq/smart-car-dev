#include "my_gpio.h"
#include <map>
#include <string>
#include <unistd.h>
#include <fcntl.h>

#define GPIO_PATH "/sys/class/gpio"

// 使用静态 map 缓存文件句柄，避免重复打开/关闭
// key: gpio_num, value: file_descriptor
static std::map<int, int> fd_cache;

/**
 * 内部辅助函数：获取或打开 GPIO 的 value 文件描述符
 */
static int get_gpio_fd(int gpio_num) {
    if (fd_cache.find(gpio_num) != fd_cache.end()) {
        return fd_cache[gpio_num];
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/gpio%d/value", GPIO_PATH, gpio_num);
    
    // 使用 O_RDWR 以兼容读写，使用底层 open 获得更高控制权
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("Failed to open GPIO value file");
        return -1;
    }
    
    fd_cache[gpio_num] = fd;
    return fd;
}

void gpio_init(int gpio_num, const char *direction, int power) {
    char path[256];
    FILE *fp;

    // 1. 导出 GPIO (此处不属于高频操作，保留原逻辑)
    snprintf(path, sizeof(path), "%s/export", GPIO_PATH);
    fp = fopen(path, "w");
    if (fp != NULL) {
        fprintf(fp, "%d", gpio_num);
        fclose(fp);
    }

    // 2. 设置方向
    snprintf(path, sizeof(path), "%s/gpio%d/direction", GPIO_PATH, gpio_num);
    fp = fopen(path, "w");
    if (fp == NULL) {
        perror("Failed to set direction");
        return;                                                            
    }
    fprintf(fp, "%s", direction);
    fclose(fp);

    // 3. 初始写入并预热缓存
    if (strcmp(direction, "out") == 0) {
        gpio_set(gpio_num, power);
    }
}

void gpio_set(int gpio_num, int power) {
    int fd = get_gpio_fd(gpio_num);
    if (fd < 0) return;

    // 优化核心：
    // 使用 pwrite 在偏移量 0 处写入，无需手动调用 lseek
    // 写入字符 '1' 或 '0' 相比 fprintf 格式化更高效
    char val = power ? '1' : '0';
    if (pwrite(fd, &val, 1, 0) < 0) {
        perror("Failed to write to GPIO");
    }
}

int gpio_get(int gpio_num) {
    int fd = get_gpio_fd(gpio_num);
    if (fd < 0) return -1;

    char val;
    if (pread(fd, &val, 1, 0) < 0) {
        perror("Failed to read from GPIO");
        return -1;
    }

    return (val == '1') ? 1 : 0;
}