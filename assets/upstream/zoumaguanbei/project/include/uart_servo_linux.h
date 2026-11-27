#ifndef UART_SERVO_LINUX_H
#define UART_SERVO_LINUX_H

#include <string>

/*
 * 嵌入式 Linux 版 JOHO 总线舵机轻量控制库
 * ------------------------------------------------------------
 * 本文件由 STM32 版 UartServoLite 移植而来，只保留“舵机位置/角度控制”。
 *
 * 协议格式：
 *   主机请求帧：FF FF ID LEN CMD PARAM... CHECKSUM
 *   舵机响应帧：FF F5 ID LEN STATUS PARAM... CHECKSUM
 *
 * 字段说明：
 *   ID       : 舵机 ID，通常为 0~254
 *   LEN      : 数据长度，等于 “ID + CMD/STATUS + PARAM” 的字节数
 *              原库中写角度时 LEN = 7，因为 ID(1) + CMD(1) + PARAM(5) = 7
 *   CMD      : 指令类型，0x01 Ping，0x02 Read，0x03 Write
 *   PARAM    : 参数区，写角度时为：寄存器地址 + 目标位置 + 运动时间
 *   CHECKSUM : 校验和，不包含两个帧头字节
 *
 * 校验和算法：
 *   checksum = ~(ID + LEN + CMD/STATUS + PARAM...) & 0xFF
 *
 * 注意：
 *   1. 本库实际用 C++ 编译，但对外接口保持接近 C 风格。
 *   2. 为了方便使用 std::string，接口参数直接使用 string。
 *   3. 本库没有创建类对象，内部只维护一个全局串口文件描述符。
 */

using std::string;

/* =========================== 串口配置 =========================== */

/* 舵机默认波特率。原 STM32 工程使用 115200。 */
#define SERVO_UART_BAUDRATE             115200

/* 接收舵机响应时的超时时间，单位 ms。原 STM32 工程为 100ms。 */
#define SERVO_UART_READ_TIMEOUT_MS      100

/* =========================== 协议常量 =========================== */

/* 主机发送给舵机的请求帧头：FF FF。 */
#define SERVO_REQ_HEADER_0              0xFF
#define SERVO_REQ_HEADER_1              0xFF

/* 舵机返回给主机的响应帧头：FF F5。 */
#define SERVO_RESP_HEADER_0             0xFF
#define SERVO_RESP_HEADER_1             0xF5

/* 指令类型。 */
#define SERVO_CMD_PING                  0x01
#define SERVO_CMD_READ                  0x02
#define SERVO_CMD_WRITE                 0x03

/* =========================== 寄存器地址 =========================== */

/* 扭矩开关寄存器：写 0x01 打开扭矩，写 0x00 关闭扭矩。 */
#define SERVO_REG_TORQUE_ENABLE         0x28

/* 目标位置寄存器：原 STM32 库通过写 0x2A 控制舵机转动到指定位置。 */
#define SERVO_REG_TARGET_POSITION       0x2A

/* 当前位置寄存器：原 STM32 库通过读 0x38、长度 2 字节获取当前位置。 */
#define SERVO_REG_PRESENT_POSITION      0x38

/*
 * 最大速度寄存器。
 * 根据实测命令 FF FF 01 04 03 47 FF B1，地址 0x47 接收一个字节的
 * 最大速度值，允许范围为 0~255。
 */
#define SERVO_REG_MAX_SPEED             0x47
#define SERVO_MAX_SPEED_MIN             0
#define SERVO_MAX_SPEED_MAX             255

/* =========================== 角度映射参数 =========================== */

/*
 * 角度范围宏。
 * 当前按常见 360° 总线舵机估算：-180° ~ +180°。
 * 如果实测舵机的机械角度范围不同，只需要修改这两个宏。
 */
#define SERVO_ANGLE_MIN_DEG             (-180.0f)
#define SERVO_ANGLE_MAX_DEG             (180.0f)

/*
 * 舵机内部位置值范围。
 * 原 STM32 示例中使用 0、2048、4095，说明位置值范围为 0~4095。
 */
#define SERVO_POSITION_MIN              0
#define SERVO_POSITION_MAX              4095

/*
 * 舵机运动时间范围。
 * 原协议中时间拆成两个字节发送，所以使用 uint16_t，单位按原库理解为 ms。
 */
#define SERVO_MOVE_TIME_MIN_MS          0
#define SERVO_MOVE_TIME_MAX_MS          65535


/*
 * 初始化舵机串口。
 *
 * 参数：
 *   port     : Linux 串口设备路径，例如："/dev/ttyS1"、"/dev/ttyUSB0"。
 *   servo_id : 默认舵机 ID。初始化时会校验 ID 合法性，并尝试打开该舵机扭矩。
 *
 * 返回：
 *   0       : 成功。
 *   负数    : 失败，通常为 -errno，例如 -EINVAL、-ENOENT、-EACCES。
 *
 * 说明：
 *   1. 若之前已经初始化过，会先自动调用 servo_clean() 关闭旧串口。
 *   2. 本函数会配置串口为 115200, 8N1, raw 模式。
 *   3. 本函数会向 servo_id 发送打开扭矩命令，但不等待响应。
 */
int servo_init(string port, int servo_id);

/*
 * 控制指定 ID 的舵机转动到目标角度。
 *
 * 参数：
 *   servo_id : 要控制的舵机 ID。
 *   angle    : 目标角度，单位为度。范围由 SERVO_ANGLE_MIN_DEG / SERVO_ANGLE_MAX_DEG 决定。
 *              超出范围会自动限幅。
 *   time_ms  : 运动时间，单位 ms。协议使用 16 位无符号数，超出范围会自动限幅。
 *
 * 协议：
 *   本函数会把 angle 映射为 0~4095 的内部位置值，然后发送：
 *   FF FF ID 07 03 2A POS_H POS_L TIME_H TIME_L CHECKSUM
 *
 * 注意：
 *   该函数为 void，发送失败时不会向外返回错误码；如果后续需要可靠控制，
 *   可以再扩展一个返回 int 的版本。
 */
void servo_angle_control(int servo_id, float angle, int time_ms);

/*
 * 设置初始化舵机的最大速度。
 *
 * 参数：
 *   servo_id : 要设置最大速度的舵机 ID，允许范围为 0~254。
 *   speed    : 最大速度值，允许范围为 0~255；超出范围会自动限幅。
 *
 * 协议：
 *   向 servo_id 指定舵机的 0x47 寄存器写入一个字节：
 *   FF FF ID 04 03 47 SPEED CHECKSUM
 *
 * 示例：
 *   servo_init("/dev/ttyS0", 1);
 *   servo_set_max_speed(1, 255);
 *   发送：FF FF 01 04 03 47 FF B1
 *
 * 注意：
 *   该函数为 void；串口未初始化时不发送数据。
 */
void servo_set_max_speed(int servo_id, int speed);

/*
 * 清理串口资源。
 *
 * 返回：
 *   0       : 成功或本来就未初始化。
 *   负数    : close() 失败时返回 -errno。
 */
int servo_clean();


#endif /* UART_SERVO_LINUX_H */
