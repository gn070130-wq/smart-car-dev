#include "cst816.h"
#include "my_gpio.h"
#include "zf_driver_pit.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <string.h>

/* 触摸信息结构体 */
TouchStructure TouchInfo;

/* I2C文件描述符 */
static int i2c_fd = -1;

/* PIT定时器轮询标志 */
static volatile bool pit_polling = false;

/**
 * @brief       打开I2C设备
 * @param       无
 * @retval      0:成功, -1:失败
 */
int cst816_open_i2c(void)
{
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) {
        perror("Failed to open I2C device");
        return -1;
    }
    
    /* 设置从机地址 */
    if (ioctl(i2c_fd, I2C_SLAVE, CST816_Write_Addr >> 1) < 0) {
        perror("Failed to set I2C slave address");
        close(i2c_fd);
        i2c_fd = -1;
        return -1;
    }
    
    return 0;
}

/**
 * @brief       关闭I2C设备
 * @param       无
 * @retval      无
 */
void cst816_close_i2c(void)
{
    if (i2c_fd >= 0) {
        close(i2c_fd);
        i2c_fd = -1;
    }
}

/**
 * @brief       触摸IC端口初始化
 * @param       无
 * @retval      无
 * @note        使用提供的gpio驱动接口
 */
void CST816_GPIOInit(void)
{
    /* 初始化RESET引脚为输出，初始高电平 */
    gpio_init(CST816_RES_PIN, "out", 1);
    
    /* 初始化INT引脚为输入 */
    gpio_init(CST816_INT_PIN, "in", 0);
}

/**
 * @brief       向触摸IC对应寄存器写入一个字节数据
 * @param       reg:寄存器地址
 * @param       dat:字节数据
 * @retval      无
 */
void CST816_SendByte(uint8_t reg, uint8_t dat)
{
    uint8_t buf[2] = {reg, dat};
    
    if (i2c_fd < 0) {
        fprintf(stderr, "I2C not opened\n");
        return;
    }
    
    if (write(i2c_fd, buf, 2) != 2) {
        perror("Failed to write to I2C");
    }
}

/**
 * @brief       向触摸IC对应寄存器连续写入len个字节数据
 * @param       reg:寄存器地址
 * @param       len:数据长度
 * @param       *buf:数据地址指针
 * @retval      无
 */
void CST816_SendData(uint8_t reg, uint32_t len, uint8_t *buf)
{
    uint8_t *write_buf = (uint8_t *)malloc(len + 1);
    if (!write_buf) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    write_buf[0] = reg;
    memcpy(&write_buf[1], buf, len);
    
    if (i2c_fd < 0) {
        fprintf(stderr, "I2C not opened\n");
        free(write_buf);
        return;
    }
    
    if (write(i2c_fd, write_buf, len + 1) != (ssize_t)(len + 1)) {
        perror("Failed to write to I2C");
    }
    
    free(write_buf);
}

/**
 * @brief       读取触摸IC对应寄存器下的数据
 * @param       reg:寄存器地址
 * @retval      寄存器对应数据
 */
uint8_t CST816_ReadByte(uint8_t reg)
{
    uint8_t value = 0;
    
    if (i2c_fd < 0) {
        fprintf(stderr, "I2C not opened\n");
        return 0;
    }
    
    /* 先写入寄存器地址 */
    if (write(i2c_fd, &reg, 1) != 1) {
        perror("Failed to write register address");
        return 0;
    }
    
    /* 然后读取数据 */
    if (read(i2c_fd, &value, 1) != 1) {
        perror("Failed to read from I2C");
        return 0;
    }
    
    return value;
}

/**
 * @brief       连续读取len个触摸IC对应寄存器下的数据
 * @param       reg:寄存器地址
 * @param       len:数据长度
 * @param       *buf:返回数据地址指针
 * @retval      无
 */
void CST816_ReadData(uint8_t reg, uint32_t len, uint8_t *buf)
{
    if (i2c_fd < 0) {
        fprintf(stderr, "I2C not opened\n");
        return;
    }
    
    /* 先写入寄存器地址 */
    if (write(i2c_fd, &reg, 1) != 1) {
        perror("Failed to write register address");
        return;
    }
    
    /* 然后读取数据 */
    if (read(i2c_fd, buf, len) != (ssize_t)len) {
        perror("Failed to read from I2C");
    }
}

/**
 * @brief       触摸IC扫描函数
 * @param       无
 * @retval      无
 * @note        移除显示方向相关代码，只保留基本触摸扫描
 */
void CST816_Scan(void)
{
    int INT_scan = gpio_get(CST816_INT_PIN);

    if (!INT_scan)
    {
        uint8_t TouchData[1 + 4 * TOUCH_MAX];                             /* 存储触摸数据 */
        CST816_ReadData(CST816_Touch_ADDR, 1 + 4 * TOUCH_MAX, TouchData); /* 读取触摸数据 */
        CST816_SendByte(CST816_Touch_ADDR, 0x00);                         /* 清除触摸标志位 */
        TouchInfo.num = TouchData[0];                                     /* 获取触摸点的个数 */
        /* 简化版本：直接读取原始坐标 */
        TouchInfo.x[0] = ((TouchData[1] & 0x0F) << 8) | TouchData[2];
        TouchInfo.y[0] = ((TouchData[3] & 0x0F) << 8) | TouchData[4];

        TouchInfo.flag = 1;
        // printf("Touch detected: x=%d, y=%d\n", TouchInfo.x[0], TouchInfo.y[0]);
    }
    else
    {
        TouchInfo.flag = 0; /* 无触摸动作 */
    }
}

/**
 * @brief       初始化触摸IC
 * @param       无
 * @retval      0:成功, -1:失败
 */
int CST816_Init(void)
{
    uint8_t TouchID, TouchConfig;
    
    /* 打开I2C设备 */
    if (cst816_open_i2c() < 0) {
        return -1;
    }
    
    /* 初始化GPIO */
    CST816_GPIOInit();
    
    /* 复位触摸芯片 */
    CST816_RES_Clr();
    usleep(10000); /* 10ms延时 */
    CST816_RES_Set();
    usleep(120000); /* 120ms延时 */
    
    /* 禁止进入低功耗模式 */
    CST816_SendByte(0xFE, 0x01);
    
    /* 读取触摸芯片ID */
    TouchID = CST816_ReadByte(0xA7);
    
    /* 读取触摸固件信息 */
    TouchConfig = CST816_ReadByte(0xA9);
    
    if (TouchID == 0xB6)
    {
        printf("Touch ID: CST816D\n");
        printf("TP Config: 0x%02X\n", TouchConfig);
        
        return 0;
    }
    else
    {
        printf("Touch error: ID=0x%02X (expected 0xB6)\n", TouchID);
        cst816_close_i2c();
        return -1;
    }
}

/**
 * @brief       测试函数
 * @param       无
 * @retval      无
 */
void CST816_Test(void)
{
    printf("CST816 Touch Test\n");
    printf("Press Ctrl+C to exit\n");
    
    /* 主循环 */
    while (1)
    {
        if (TouchInfo.flag)
        {
            printf("Touch: x=%d, y=%d\n", TouchInfo.x[0], TouchInfo.y[0]);
            TouchInfo.flag = 0;
        }
        usleep(100000); /* 100ms */
    }
}