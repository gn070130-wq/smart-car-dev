#include "fps.hpp"
#include <cstdio>
#include <sys/time.h>
#include "zf_common_headfile.hpp"
// 静态全局变量（仅在本文件可见，避免重定义）
static uint64_t _last_time_ms = 0;
static int _frame_count = 0;

// 内部工具函数：获取系统毫秒时间（龙芯2K0300 Linux通用）
static uint64_t get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<uint64_t>(tv.tv_sec) * 1000 + static_cast<uint64_t>(tv.tv_usec) / 1000;
}

// 初始化：仅声明的实现
void fps_init(void)
{
    _last_time_ms = get_time_ms();
    _frame_count = 0;
}

// 每帧更新：仅声明的实现
void fps_update(void)
{
    _frame_count++;
    const uint64_t now = get_time_ms();

    // 每1秒打印一次真实FPS
    if (now - _last_time_ms >= 1000)
    {
        printf("[FPS] %d\n", _frame_count);
        _frame_count = 0;
        _last_time_ms = now;
    }
}