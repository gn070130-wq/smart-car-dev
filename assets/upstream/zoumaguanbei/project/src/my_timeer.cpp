#include "my_timeer.h"
#include <sys/resource.h>
#include <chrono>
#include <thread>
#include <sys/time.h>
#include <time.h>
#include <turbojpeg.h>

// 获取当前进程消耗的 CPU 时间（纳秒）
uint64_t get_cpu_time_ns()
{
    struct timespec ts;
    // CLOCK_PROCESS_CPUTIME_ID 仅计算当前进程及其线程消耗的 CPU 时间
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == -1)
        return 0;
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// 获取系统单调时间（纳秒），不受系统时间修改影响
uint64_t get_wall_time_ns()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
        return 0;
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}