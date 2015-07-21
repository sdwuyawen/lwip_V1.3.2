#ifndef  __LWIP_DEMO_H
#define  __LWIP_DEMO_H

//#include "mysys.h"
//#include "tcp_server.h"
//#include "tcp_client.h"
//#include "udp_server.h"
//#include "udp_client.h"


#define LWIP_CONNECTED  0X80 //连接成功
#define LWIP_NEW_DATA   0x40 //有新的数据
#define LWIP_SEND_DATA  0x20 //有数据需要发送
#define LWIP_DEMO_BUF        200

#define LWIP_TCP_SERVER 0x80  //tcp 服务器功能
#define LWIP_TCP_CLIENT 0x40	//tcp 客户端功能
#define LWIP_UDP_SERVER 0x20  //UDP 服务器功能
#define LWIP_UDP_CLIENT 0x10  //UDP 客户端功能
#define LWIP_WEBSERVER  0x08  //UDP 客户端功能


#define LWIP_DEMO_DEBUG 1  //是否打印调试信息   

extern u8 lwip_flag;


extern u8 lwip_test_mode;
extern u8 lwip_demo_buf[];

void lwip_demo_init(void);	//初始化LWIP_DEMO
void lwip_log(char *str);  //打印应用程序状态

#endif

