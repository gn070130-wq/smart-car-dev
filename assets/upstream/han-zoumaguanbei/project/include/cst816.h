#ifndef _CST816_H_
#define _CST816_H_

#include <stdint.h>
#include <stdbool.h>

/* 从机地址 */
#define CST816_Write_Addr 0x2A
#define CST816_Read_Addr 0x2B

/* 端口定义 - 使用Linux GPIO编号 */
#define CST816_RES_PIN 54
#define CST816_INT_PIN 55

/* 定义端口电平状态 - 使用提供的gpio驱动接口 */
#define CST816_RES_Set() gpio_set(CST816_RES_PIN, 1)
#define CST816_RES_Clr() gpio_set(CST816_RES_PIN, 0)

#define CST816_INT_Set() gpio_set(CST816_INT_PIN, 1)
#define CST816_INT_Clr() gpio_set(CST816_INT_PIN, 0)

/* 中断配置 - 改为轮询，读取GPIO状态 */
#define CST816_INT gpio_get(CST816_INT_PIN)

/* 定义支持触摸点个数 */
#define TOUCH_MAX 1

/* 定义触摸信息结构体 */
typedef struct
{
    uint8_t flag;          /* 触摸状态标志位 */
    uint8_t num;           /* 触摸点个数 */
    uint16_t x[TOUCH_MAX]; /* 触摸坐标点信息 */
    uint16_t y[TOUCH_MAX]; /* 触摸坐标点信息 */
} TouchStructure;

extern TouchStructure TouchInfo;

/* 触摸相关寄存器 */
#define CST816_ID_ADDR 0xA7
#define CST816_CFG_ADDR 0xA9
#define CST816_Touch_ADDR 0x02

/* 函数声明 */
void CST816_GPIOInit(void);
void CST816_SendByte(uint8_t reg, uint8_t dat);
void CST816_SendData(uint8_t reg, uint32_t len, uint8_t *buf);
uint8_t CST816_ReadByte(uint8_t reg);
void CST816_ReadData(uint8_t reg, uint32_t len, uint8_t *buf);
void CST816_Scan(void);
int CST816_Init(void);
void CST816_Test(void);

/* Linux特定函数 */
int cst816_open_i2c(void);
void cst816_close_i2c(void);
void cst816_poll_task(void);

/* PIT定时器轮询函数 */
void cst816_pit_poll(void);
void cst816_pit_task_init(void);
void cst816_pit_task_stop(void);

#endif