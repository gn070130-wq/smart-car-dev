#include "adc.h"

#define ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage"

int get_adc_sum(int adc_num){

    char path[256];
    FILE *fp;
    int adc_sum;
    snprintf(path, sizeof(path), "%s%d_raw", ADC_PATH, adc_num);
    fp = fopen(path, "r");
    if(fscanf(fp, "%d", &adc_sum) != 1) {
        perror("Failed to read value from file");
        fclose(fp);
        return -1;  // 错误处理，返回 -1 表示读取失败
    }

    // 关闭文件
    fclose(fp);

    // 返回读取的值
    return adc_sum;
}

float get_battery_voltage(){

    return get_adc_sum(7) * 1.8 / 4096.0 / 0.045;
}