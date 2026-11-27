#ifndef MY_TIMEER_H
#define MY_TIMEER_H

#include <stdint.h>

uint64_t get_cpu_time_ns();
uint64_t get_wall_time_ns();

#endif // MY_TIMEER_H