#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// 在头文件或模块顶部添加
#define FANS_IOCTL_MAGIC    'F'
#define FANS_SET_DUTY    _IOW(FANS_IOCTL_MAGIC, 1, struct Fans_duty)

struct Fans_duty
{
    int dutyl;
    int dutyr;
};

int main()
{
    int fd = open("/dev/wuwu_fans", O_WRONLY);
    if(fd < 0) {
        perror("Error open fans\r\n");
        return 0;
    }

    struct Fans_duty fans;
    fans.dutyl = 110;
    fans.dutyr = 20;
    int err = ioctl(fd, FANS_SET_DUTY, &fans);
    if(err < 0) {
        perror("Failed to set fans duty via ioctl\r\n");
        return 0;
    }
    while(1){};
    close(fd);
    return 0;
}