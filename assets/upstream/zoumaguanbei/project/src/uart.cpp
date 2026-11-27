#include "zf_common_headfile.h"
#include "pwm.h"
#include "cst816.h"

int uart_fd = -1;
int use_data_send[9] = {0};
extern int le[];
extern int re[];
extern int accel_num[];
extern int gyro_num[];
extern int16 encoder_left;
extern int16 encoder_right;
extern int slow_speed;
extern int key_scan_gpio;
extern int left_expect_speed;
extern int right_expect_speed;
extern int expect_speed;
extern int pwm_left;
//extern int pwm_right;

// 打开并配置串口
bool uart_init(const char* device, int baudrate) {
    uart_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd < 0) {
        perror("打开串口失败");
        return false;
    }

    termios options{};
    tcgetattr(uart_fd, &options);

    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);

    options.c_cflag |= (CLOCAL | CREAD);    // 打开接收器，设置本地模式
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;                 // 8位数据位
    options.c_cflag &= ~PARENB;             // 无校验位
    options.c_cflag &= ~CSTOPB;             // 1位停止位
    options.c_cflag &= ~CRTSCTS;            // 无流控

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 原始输入
    options.c_iflag &= ~(IXON | IXOFF | IXANY);         // 关闭软件流控
    options.c_oflag &= ~OPOST;                          // 原始输出

    tcsetattr(uart_fd, TCSANOW, &options);
    return true;
}

// 写一个字节
void uart_write_byte(unsigned char byte) {
    write(uart_fd, &byte, 1);
}

// 数据发送函数，参数为 9 个整数
void Data_Send(int* pst) {
    unsigned char i = 0;
    unsigned char _cnt = 0;
    unsigned char sum = 0;
    unsigned char data_to_send[32];

    data_to_send[_cnt++] = 0xAA;
    data_to_send[_cnt++] = 0xAA;
    data_to_send[_cnt++] = 0x02;
    data_to_send[_cnt++] = 0;

    for (int j = 0; j < 8; ++j) {
        data_to_send[_cnt++] = (unsigned char)(pst[j] >> 8); // 高位
        data_to_send[_cnt++] = (unsigned char)(pst[j]);      // 低位
    }

    data_to_send[3] = _cnt - 4;

    for (i = 0; i < _cnt; i++) {
        sum += data_to_send[i];
    }

    data_to_send[_cnt++] = sum;

    // 发送数据
    for (i = 0; i < _cnt; i++) {
        uart_write_byte(data_to_send[i]);
    }
}

void pit_callback_speed_uart()
{
    //encoder_left  = kalman_filter_int(encoder_get_count(ENCODER_LEFT));
    //encoder_right = kalman_filter_int(-encoder_get_count(ENCODER_RIGHT));
    encoder_left = encoder_get_count(ENCODER_LEFT);
    encoder_right = -encoder_get_count(ENCODER_RIGHT);
    speed_control(encoder_left, encoder_right, left_expect_speed, right_expect_speed);
    use_data_send[0] = encoder_left;
    use_data_send[1] = encoder_right;
    use_data_send[2] = left_expect_speed;
    use_data_send[3] = right_expect_speed;
    //use_data_send[4] = pwm_left;
    Data_Send(use_data_send);
}

int test_uart()
{
    if (!uart_init("/dev/ttyS0"))
    {
        return 1;
    }
    /* int speed = 0;
    // 模拟数据 */
    pit_ms_clean();
    pit_ms_init(5, pit_callback_speed_uart);
    left_expect_speed = right_expect_speed = 0;
    key_scan_gpio = keyboard_scan_gpio();
    while(key_scan_gpio != ESC) {

        key_scan_gpio = keyboard_scan_cst816();
        if (key_scan_gpio == LEFT) {
            left_expect_speed = right_expect_speed = 0;
            
        }
        else if (key_scan_gpio == DOWN) {
            left_expect_speed = right_expect_speed = 200;
           
        }
        else if (key_scan_gpio == RIGHT){
            left_expect_speed = right_expect_speed = 400;
            
        }
       
    }
    pit_ms_clean();
    close(uart_fd);
    return 0;
}