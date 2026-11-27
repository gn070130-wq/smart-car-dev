#ifndef TEMPERATURE_H
#define TEMPERATURE_H

// 读取 CPU 温度，单位为毫摄氏度（例如 39640 表示 39.640°C）。
// 读取失败时返回 -1。
int get_cpu_temperature_millicelsius();

// 读取 CPU 温度，单位为摄氏度。
// 读取失败时返回 -1.0f。
float get_cpu_temperature();

#endif // TEMPERATURE_H
