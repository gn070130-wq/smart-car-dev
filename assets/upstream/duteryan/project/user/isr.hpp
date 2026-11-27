#ifndef __ISR_hpp__
#define __ISR_hpp__
#include  "zf_common_headfile.hpp"
extern zf_driver_pit pit_timer;
void isr1_handler();
void pit_callback();
void sigint_handler(int signum);
void cleanup();
void pit_init();
void begin_runtime_reset_guard();//主线程清状态前调用：等待当前PIT回调退出并阻止新回调访问状态
void end_runtime_reset_guard();//主线程完成清状态后调用
void reset_isr_runtime_state();//PIT已被reset guard隔离时调用
#endif
