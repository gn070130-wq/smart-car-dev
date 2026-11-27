#include "uart_servo_linux.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

/*
 * 嵌入式 Linux 版 JOHO 总线舵机轻量控制库实现
 * ------------------------------------------------------------
 * 设计目标：
 *   1. 尽量保持原 STM32 UartServoLite 的协议行为。
 *   2. 只依赖 Linux 标准系统调用和 termios，不引入第三方库。
 *   3. 对外接口简单，内部隐藏串口细节。
 *
 * 本文件只实现舵机位置/角度控制，不包含电机模式。
 */

namespace {

/*
 * 全局串口文件描述符。
 *
 * Linux 下串口设备也是文件，例如 /dev/ttyS1、/dev/ttyUSB0。
 * open() 成功后会得到一个 fd，之后 read/write 都通过这个 fd 操作。
 *
 * g_fd < 0 表示当前未初始化或已经清理。
 */
int g_fd = -1;

/*
 * 将普通整数波特率转换为 termios 使用的 speed_t 常量。
 *
 * 当前头文件默认 SERVO_UART_BAUDRATE 为 115200。
 * 如果后期需要改成其他波特率，可同时扩展这里的 switch。
 */
speed_t baud_to_termios(int baud)
{
    switch (baud) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
#ifdef B230400
    case 230400: return B230400;
#endif
#ifdef B460800
    case 460800: return B460800;
#endif
    default: return B115200;
    }
}

/*
 * 检查舵机 ID 是否在协议允许范围内。
 *
 * 原 STM32 头文件定义 JOHO_US_NUM 为 254，因此这里使用 0~254。
 * 实际项目中如果 0 是广播 ID 或保留 ID，可按舵机手册进一步收紧范围。
 */
bool valid_servo_id(int id)
{
    return id >= 0 && id <= 254;
}

/*
 * 计算 JOHO 协议校验和。
 *
 * 输入 frame_without_checksum 必须已经包含：
 *   FF FF ID LEN CMD PARAM...
 * 或：
 *   FF F5 ID LEN STATUS PARAM...
 *
 * 校验时跳过两个帧头字节，从 ID 开始累加：
 *   checksum = ~(ID + LEN + CMD/STATUS + PARAM...) & 0xFF
 */
uint8_t checksum(const std::vector<uint8_t>& frame_without_checksum)
{
    uint32_t sum = 0;
    for (size_t i = 2; i < frame_without_checksum.size(); ++i) {
        sum += frame_without_checksum[i];
    }
    return static_cast<uint8_t>((~sum) & 0xFF);
}

/*
 * 保证把指定长度的数据全部写入串口。
 *
 * write() 不保证一次写完所有数据，所以这里循环写，直到全部写出或发生错误。
 * 返回：
 *   0       : 成功。
 *   负数    : 失败，值为 -errno。
 */
int write_all(const uint8_t* data, size_t len)
{
    if (g_fd < 0) return -ENODEV;

    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::write(g_fd, data + sent, len - sent);
        if (n > 0) {
            sent += static_cast<size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        return -errno;
    }
    return 0;
}

/*
 * 构造并发送一帧 JOHO 请求包。
 *
 * 帧格式：
 *   FF FF ID LEN CMD PARAM... CHECKSUM
 *
 * LEN 的定义与原 STM32 工程一致：
 *   LEN = ID(1) + CMD(1) + PARAM(n) = n + 2
 *
 * 参数：
 *   servo_id : 舵机 ID。
 *   cmd      : 指令类型，例如 SERVO_CMD_WRITE。
 *   params   : 参数区，不包含 ID、LEN、CMD、CHECKSUM。
 */
int send_packet(int servo_id, uint8_t cmd, const std::vector<uint8_t>& params)
{
    if (!valid_servo_id(servo_id)) return -EINVAL;
    if (params.size() > 253) return -EMSGSIZE;

    const uint8_t len = static_cast<uint8_t>(params.size() + 2);

    std::vector<uint8_t> frame;
    frame.reserve(params.size() + 6);
    frame.push_back(SERVO_REQ_HEADER_0);
    frame.push_back(SERVO_REQ_HEADER_1);
    frame.push_back(static_cast<uint8_t>(servo_id));
    frame.push_back(len);
    frame.push_back(cmd);
    frame.insert(frame.end(), params.begin(), params.end());
    frame.push_back(checksum(frame));

    return write_all(frame.data(), frame.size());
}

/*
 * 配置 Linux 串口为原 STM32 工程等价的数据格式：115200, 8N1。
 *
 * 主要配置项：
 *   raw 模式       : 不让内核处理换行、回显、特殊控制字符等。
 *   8 数据位       : CS8。
 *   无校验         : 关闭 PARENB。
 *   1 停止位       : 关闭 CSTOPB。
 *   关闭硬件流控   : 关闭 CRTSCTS。
 *   非阻塞读取     : VMIN=0, VTIME=0，超时由 poll() 控制。
 */
int configure_serial(int fd)
{
    termios tio{};
    if (::tcgetattr(fd, &tio) != 0) return -errno;

    ::cfmakeraw(&tio);
    ::cfsetispeed(&tio, baud_to_termios(SERVO_UART_BAUDRATE));
    ::cfsetospeed(&tio, baud_to_termios(SERVO_UART_BAUDRATE));

    tio.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);
    tio.c_cflag &= static_cast<tcflag_t>(~PARENB);
    tio.c_cflag &= static_cast<tcflag_t>(~CSTOPB);
    tio.c_cflag &= static_cast<tcflag_t>(~CSIZE);
    tio.c_cflag |= CS8;
#ifdef CRTSCTS
    tio.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
#endif

    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if (::tcsetattr(fd, TCSANOW, &tio) != 0) return -errno;

    /* 清空历史收发缓冲，避免初始化前残留数据影响后续通信。 */
    ::tcflush(fd, TCIOFLUSH);
    return 0;
}

/*
 * 将角度值转换为舵机内部位置值。
 *
 * 原 STM32 示例说明：
 *   0      -> 一端位置
 *   2048   -> 中间位置
 *   4095   -> 另一端位置
 *
 * 这里默认映射为：
 *   SERVO_ANGLE_MIN_DEG -> SERVO_POSITION_MIN
 *   SERVO_ANGLE_MAX_DEG -> SERVO_POSITION_MAX
 *
 * 如果实测方向相反，可以交换 SERVO_ANGLE_MIN_DEG / SERVO_ANGLE_MAX_DEG，
 * 或在此函数中改成反向映射。
 */
uint16_t angle_to_position(float angle)
{
    angle = std::max(SERVO_ANGLE_MIN_DEG, std::min(SERVO_ANGLE_MAX_DEG, angle));

    const float ratio = (angle - SERVO_ANGLE_MIN_DEG) /
                        (SERVO_ANGLE_MAX_DEG - SERVO_ANGLE_MIN_DEG);
    const float pos_f = SERVO_POSITION_MIN +
                        ratio * (SERVO_POSITION_MAX - SERVO_POSITION_MIN);

    int pos = static_cast<int>(pos_f + 0.5f);
    pos = std::max(SERVO_POSITION_MIN, std::min(SERVO_POSITION_MAX, pos));
    return static_cast<uint16_t>(pos);
}

/*
 * 将外部传入的运动时间限制到协议允许范围。
 *
 * 原库把 interval 拆为两个字节发送，所以最大值是 65535。
 */
uint16_t clamp_time_ms(int time_ms)
{
    time_ms = std::max(SERVO_MOVE_TIME_MIN_MS, std::min(SERVO_MOVE_TIME_MAX_MS, time_ms));
    return static_cast<uint16_t>(time_ms);
}

/*
 * 控制舵机扭矩开关。
 *
 * 参数：
 *   enable = true  : 写 0x28 0x01，打开扭矩，舵机可以主动保持/运动。
 *   enable = false : 写 0x28 0x00，关闭扭矩，舵机轴通常可以被外力转动。
 */
void set_torque(int servo_id, bool enable)
{
    std::vector<uint8_t> params;
    params.push_back(SERVO_REG_TORQUE_ENABLE);
    params.push_back(enable ? 0x01 : 0x00);
    send_packet(servo_id, SERVO_CMD_WRITE, params);
}

} // namespace

int servo_init(string port, int servo_id)
{
    if (!valid_servo_id(servo_id) || port.empty()) return -EINVAL;

    /* 如果已经初始化过，先释放旧串口，避免 fd 泄漏。 */
    if (g_fd >= 0) {
        servo_clean();
    }

    /*
     * 打开串口设备。
     * O_RDWR     : 可读可写。
     * O_NOCTTY   : 不把该串口作为当前进程的控制终端。
     * O_NONBLOCK : 非阻塞模式，后续用 poll() 自己控制超时。
     */
    int fd = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return -errno;

    int rc = configure_serial(fd);
    if (rc < 0) {
        ::close(fd);
        return rc;
    }

    g_fd = fd;
    /* 初始化完成后默认打开扭矩，使舵机能够响应位置控制命令。 */
    set_torque(servo_id, true);

    return 0;
}

void servo_angle_control(int servo_id, float angle, int time_ms)
{
    if (g_fd < 0 || !valid_servo_id(servo_id)) return;

    const uint16_t pos = angle_to_position(angle);
    const uint16_t move_time = clamp_time_ms(time_ms);

    /*
     * 写目标位置命令参数区：
     *   PARAM[0] = 0x2A              目标位置寄存器
     *   PARAM[1] = POS_H            目标位置高字节
     *   PARAM[2] = POS_L            目标位置低字节
     *   PARAM[3] = TIME_H           运动时间高字节
     *   PARAM[4] = TIME_L           运动时间低字节
     *
     * 注意：
     *   原 STM32 库虽然整体说明提到小端，但位置和时间在 USL_SetServoAngle()
     *   中是手动按高字节在前发送的，因此这里保持完全一致。
     */
    std::vector<uint8_t> params;
    params.reserve(5);
    params.push_back(SERVO_REG_TARGET_POSITION);
    params.push_back(static_cast<uint8_t>((pos >> 8) & 0xFF));
    params.push_back(static_cast<uint8_t>(pos & 0xFF));
    params.push_back(static_cast<uint8_t>((move_time >> 8) & 0xFF));
    params.push_back(static_cast<uint8_t>(move_time & 0xFF));

    send_packet(servo_id, SERVO_CMD_WRITE, params);
}

void servo_set_max_speed(int servo_id, int speed)
{
    /* 串口尚未初始化或舵机 ID 无效时，不发送任何数据。 */
    if (g_fd < 0 || !valid_servo_id(servo_id)) return;

    /*
     * 对调用者传入的值进行限幅，确保协议参数始终是有效的单字节值。
     * speed < 0 时写入 0；speed > 255 时写入 255。
     */
    speed = std::max(SERVO_MAX_SPEED_MIN, std::min(SERVO_MAX_SPEED_MAX, speed));

    /*
     * 参数区包含寄存器地址和一个速度字节：
     *   PARAM[0] = 0x47
     *   PARAM[1] = speed
     *
     * 因此 LEN = ID(1) + CMD(1) + PARAM(2) = 4。
     * 当 servo_id=1、speed=255 时，完整帧为：
     *   FF FF 01 04 03 47 FF B1
     */
    std::vector<uint8_t> params;
    params.reserve(2);
    params.push_back(SERVO_REG_MAX_SPEED);
    params.push_back(static_cast<uint8_t>(speed));

    send_packet(servo_id, SERVO_CMD_WRITE, params);
}

int servo_clean()
{
    int rc = 0;

    if (g_fd >= 0) {
        /* 等待串口发送缓冲中的数据尽量发完，再关闭 fd。 */
        ::tcdrain(g_fd);

        if (::close(g_fd) != 0) rc = -errno;
        g_fd = -1;
    }

    return rc;
}
