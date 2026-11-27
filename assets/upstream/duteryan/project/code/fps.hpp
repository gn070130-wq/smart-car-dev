#ifndef FPS_HPP
#define FPS_HPP

#include <cstdint>

// 初始化帧率统计（仅声明）
void fps_init(void);

// 每帧调用一次，自动计算并打印FPS（仅声明）
void fps_update(void);

#endif // FPS_HPP