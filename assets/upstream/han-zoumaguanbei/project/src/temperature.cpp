#include "temperature.h"

#include <cerrno>
#include <cstdio>

#define CPU_TEMPERATURE_PATH "/sys/class/thermal/thermal_zone0/temp"

int get_cpu_temperature_millicelsius()
{
    FILE *fp = fopen(CPU_TEMPERATURE_PATH, "r");
    if (fp == nullptr) {
        perror("Failed to open CPU temperature file");
        return -1;
    }

    int temperature_millicelsius;
    if (fscanf(fp, "%d", &temperature_millicelsius) != 1) {
        if (ferror(fp) && errno != 0)
            perror("Failed to read CPU temperature");
        else
            fprintf(stderr, "Failed to parse CPU temperature from %s\n",
                    CPU_TEMPERATURE_PATH);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return temperature_millicelsius;
}

float get_cpu_temperature()
{
    const int temperature_millicelsius = get_cpu_temperature_millicelsius();
    if (temperature_millicelsius < 0)
        return -1.0f;

    return temperature_millicelsius / 1000.0f;
}
