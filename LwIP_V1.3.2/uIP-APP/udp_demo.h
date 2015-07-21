#ifndef __UDP_DEMO_H__
#define __UDP_DEMO_H__		 
/* Since this file will be included by uip.h, we cannot include uip.h
   here. But we might need to include uipopt.h if we need the u8_t and
   u16_t datatypes. */
#include "uipopt.h"
#include "psock.h"
#include "stm32f10x.h"
 
//定义 uip_tcp_appstate_t 数据类型，用户可以添加应用程序需要用到
//成员变量。不要更改结构体类型的名字，因为这个类型名会被uip引用。
//uip.h 中定义的 	struct uip_conn  结构体中引用了 uip_tcp_appstate_t		  
struct udp_demo_appstate
{
	u8_t state;
	u8_t *textptr;
	int textlen;
};	 
typedef struct udp_demo_appstate uip_udp_appstate_t;


extern u8 udp_server_databuf[200];   	//发送数据缓存	  
extern u8 udp_server_sta;				//服务端状态


//定义应用程序回调函数 
#ifndef UIP_UDP_APPCALL
#define UIP_UDP_APPCALL udp_demo_appcall //定义回调函数为 udp_demo_appcall 
#endif


#endif
























