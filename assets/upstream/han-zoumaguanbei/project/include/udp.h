#ifndef UDP_H
#define UDP_H

#include <cstdint>

bool udp_init(const char *host_ip = "192.168.1.100", uint16_t host_port = 9000);
void udp_send_telemetry();
void pit_callback_speed_udp();
int test_udp(const char *host_ip = "192.168.1.100", uint16_t host_port = 9000);

#endif // UDP_H
