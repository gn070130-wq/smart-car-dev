#ifndef ADC_H
#define ADC_H

#include <iostream>        // 必须：std::cout, cerr
#include <cstdint>         // 必须：uint8_t

// 获取ADC值
int get_adc_sum(int adc_num);

float get_battery_voltage();

#endif // ADC_H