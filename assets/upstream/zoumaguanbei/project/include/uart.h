#ifndef UART_H
#define UART_H

// 打开并配置串口
bool uart_init(const char* device, int baudrate = B115200);

// 写一个字节到串口
void uart_write_byte(unsigned char byte);

// 数据发送函数，参数为 9 个整数
void Data_Send(int* pst);

// 测试 UART 
int test_uart();

#endif // UART_H
