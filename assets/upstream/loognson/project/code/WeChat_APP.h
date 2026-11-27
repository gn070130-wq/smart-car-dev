#ifndef _WECHAT_APP_H_
#define _WECHAT_APP_H_

#include "zf_common_headfile.hpp"

/****************************** UDP ******************************/
#define USE_WECHAT_APP      (0)                 // 是否使用微信小程序
#define WECHAT_UDP_PORT     (8888)              // 龙芯UDP监听端口号
#define WECHAT_SERVER_IP    ("192.168.43.1")    // 微信小程序服务器IP
#define WECHAT_RECV_BUFFER_SIZE  (256)          // 微信小程序接收缓冲区大小

void WeChat_UDP_init(void);

#endif
 