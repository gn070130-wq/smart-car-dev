#include "WeChat_APP.h"

static zf_driver_udp wechat_udp;
static uint8_t wechat_recv_buffer[WECHAT_RECV_BUFFER_SIZE];

void WeChat_UDP_init(void)
{
    wechat_udp.init(WECHAT_SERVER_IP, WECHAT_UDP_PORT);
    printf("Waiting for WIFI init......\r\n");
    while(1)
    {
        memset(wechat_recv_buffer, 0, WECHAT_RECV_BUFFER_SIZE);
        uint32_t recv_len = wechat_udp.read_data(wechat_recv_buffer, WECHAT_RECV_BUFFER_SIZE);
        if(recv_len > 0)
        {
            if(strcmp((const char*)wechat_recv_buffer, "WIFI_CONNECTED") == 0)
            {
                printf("WIFI connected\r\n");
                break;
            }
        }
    }
}
