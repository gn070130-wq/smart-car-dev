#include "zf_common_headfile.h"
#include "control.h"
#include "udp.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

namespace {
constexpr uint8_t FRAME_HEAD = 0xAA;
constexpr uint8_t FRAME_TYPE_TELEMETRY = 0x02;
constexpr uint8_t FRAME_TYPE_COMMAND = 0x03;
constexpr uint16_t UDP_PORT_LOCAL = 9001;
constexpr uint16_t UDP_PORT_HOST = 9000;
constexpr char UDP_HOST_DEFAULT[] = "192.168.1.100";
int udp_fd = -1;
sockaddr_in host_addr{};

uint32_t read_u32_be(const uint8_t *p) {
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | p[3];
}
void write_u32_be(uint8_t *p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v >> 24); p[1] = static_cast<uint8_t>(v >> 16);
    p[2] = static_cast<uint8_t>(v >> 8); p[3] = static_cast<uint8_t>(v);
}
void write_i32_be(uint8_t *p, int32_t v) { write_u32_be(p, static_cast<uint32_t>(v)); }
void write_float_be(uint8_t *p, float v) {
    uint32_t bits; std::memcpy(&bits, &v, sizeof(bits)); write_u32_be(p, bits);
}
float read_float_be(const uint8_t *p) {
    uint32_t bits = read_u32_be(p); float v; std::memcpy(&v, &bits, sizeof(v)); return v;
}
uint8_t checksum(const uint8_t *p, size_t n) {
    uint8_t sum = 0; for (size_t i = 0; i < n; ++i) sum += p[i]; return sum;
}
}

extern int left_expect_speed, right_expect_speed;
extern int key_scan_gpio;
extern int16 encoder_left, encoder_right;
extern int pwm_left, pwm_right;
extern float wp, wi, wd;

bool udp_init(const char *host_ip, uint16_t host_port) {
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) return false;
    int reuse = 1; setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in local{}; local.sin_family = AF_INET; local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(UDP_PORT_LOCAL);
    if (bind(udp_fd, reinterpret_cast<sockaddr *>(&local), sizeof(local)) < 0) {
        close(udp_fd); udp_fd = -1; return false;
    }
    int flags = fcntl(udp_fd, F_GETFL, 0); fcntl(udp_fd, F_SETFL, flags | O_NONBLOCK);
    host_addr.sin_family = AF_INET; host_addr.sin_port = htons(host_port);
    if (inet_aton(host_ip ? host_ip : UDP_HOST_DEFAULT, &host_addr.sin_addr) == 0) {
        close(udp_fd); udp_fd = -1; return false;
    }
    return true;
}

void udp_send_telemetry() {
    if (udp_fd < 0) return;
    uint8_t frame[64]{}; size_t n = 0;
    frame[n++] = FRAME_HEAD; frame[n++] = FRAME_HEAD; frame[n++] = FRAME_TYPE_TELEMETRY; frame[n++] = 36;
    write_i32_be(frame + n, encoder_left); n += 4; write_i32_be(frame + n, encoder_right); n += 4;
    write_i32_be(frame + n, left_expect_speed); n += 4; write_i32_be(frame + n, right_expect_speed); n += 4;
    write_i32_be(frame + n, pwm_left); n += 4; write_i32_be(frame + n, pwm_right); n += 4;
    write_float_be(frame + n, wp); n += 4; write_float_be(frame + n, wi); n += 4; write_float_be(frame + n, wd); n += 4;
    frame[n++] = checksum(frame, n);
    sendto(udp_fd, frame, n, 0, reinterpret_cast<sockaddr *>(&host_addr), sizeof(host_addr));
}

static void udp_receive_commands() {
    if (udp_fd < 0) return;
    uint8_t frame[64]; sockaddr_in from{}; socklen_t from_len = sizeof(from);
    for (;;) {
        ssize_t got = recvfrom(udp_fd, frame, sizeof(frame), MSG_DONTWAIT,
                               reinterpret_cast<sockaddr *>(&from), &from_len);
        if (got < 0) { if (errno == EAGAIN || errno == EWOULDBLOCK) break; return; }
        if (got != 25 || frame[0] != FRAME_HEAD || frame[1] != FRAME_HEAD ||
            frame[2] != FRAME_TYPE_COMMAND || frame[3] != 20 || checksum(frame, 24) != frame[24]) continue;
        left_expect_speed = static_cast<int32_t>(read_u32_be(frame + 4));
        right_expect_speed = static_cast<int32_t>(read_u32_be(frame + 8));
        wp = read_float_be(frame + 12); wi = read_float_be(frame + 16); wd = read_float_be(frame + 20);
    }
}

void pit_callback_speed_udp() {
    udp_receive_commands();
    encoder_left = encoder_get_count(ENCODER_LEFT);
    encoder_right = -encoder_get_count(ENCODER_RIGHT);
    speed_control(encoder_left, encoder_right, left_expect_speed, right_expect_speed);
    udp_send_telemetry();
}

int test_udp(const char *host_ip, uint16_t host_port) {
    if (!udp_init(host_ip, host_port)) return 1;
    left_expect_speed = right_expect_speed = 0;
    pit_ms_clean(); pit_ms_init(5, pit_callback_speed_udp);
    key_scan_gpio = keyboard_scan_gpio();
    while (key_scan_gpio != ENTER) key_scan_gpio = keyboard_scan_cst816();
    pit_ms_clean(); close(udp_fd); udp_fd = -1; return 0;
}
