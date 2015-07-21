#include "lwip_demo.h"

#include "httpd.h"
#include "ssi_cgi_handle.h"
#include "usart.h"



u8 lwip_demo_buf[LWIP_DEMO_BUF];		//�����������ͺͽ������ݵĻ���
u8 lwip_flag;      //���ڶ���lwip״̬
 


u8 lwip_test_mode = LWIP_TCP_SERVER;		//��ǰ���ԵĹ���



//��ʼ��lwip_demo
void lwip_demo_init(void){
			switch(lwip_test_mode){
				case LWIP_TCP_SERVER: 
					Init_TCP_Server();							//��ʼ��TCP����������Ҫ�趨�ñ���IP��ַ�Ͷ˿ں�
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
	       httpd_init();				//��ʼ��webserver
	       init_ssi_cgi();
					break;				
				default:break;
			}
}

//lwipӦ�ó���״̬��־
void lwip_log(char *str){
	#if LWIP_DEMO_DEBUG>0
	printf("lwip:%s\r\n",str);
	#endif
}

