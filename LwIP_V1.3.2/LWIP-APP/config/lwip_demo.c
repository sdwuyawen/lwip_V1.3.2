#include "lwip_demo.h"

#include "httpd.h"
#include "ssi_cgi_handle.h"
#include "usart.h"



u8 lwip_demo_buf[LWIP_DEMO_BUF];		//定义用来发送和接收数据的缓存
u8 lwip_flag;      //用于定义lwip状态
 


u8 lwip_test_mode = LWIP_TCP_SERVER;		//当前测试的功能



//初始化lwip_demo
void lwip_demo_init(void){
			switch(lwip_test_mode){
				case LWIP_TCP_SERVER: 
					Init_TCP_Server();							//初始化TCP服务器，需要设定好本地IP地址和端口号
					break;				
				case LWIP_TCP_CLIENT: 
					Init_TCP_Client();	
					break;
				case LWIP_UDP_SERVER: 
					Init_UDP_Server();
					break;		
				case LWIP_UDP_CLIENT: 
					Init_UDP_Client();
					break;		
				case LWIP_WEBSERVER: 
	       httpd_init();				//初始化webserver
	       init_ssi_cgi();
					break;				
				default:break;
			}
}

//lwip应用程序状态日志
void lwip_log(char *str){
	#if LWIP_DEMO_DEBUG>0
	printf("lwip:%s\r\n",str);
	#endif
}

