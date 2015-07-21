#ifndef __UDP_DEMO_H__
#define __UDP_DEMO_H__		 
/* Since this file will be included by uip.h, we cannot include uip.h
   here. But we might need to include uipopt.h if we need the u8_t and
   u16_t datatypes. */
#include "uipopt.h"
#include "psock.h"
#include "stm32f10x.h"
 
//���� uip_tcp_appstate_t �������ͣ��û��������Ӧ�ó�����Ҫ�õ�
//��Ա��������Ҫ���Ľṹ�����͵����֣���Ϊ����������ᱻuip���á�
//uip.h �ж���� 	struct uip_conn  �ṹ���������� uip_tcp_appstate_t		  
struct udp_demo_appstate
{
	u8_t state;
	u8_t *textptr;
	int textlen;
};	 
typedef struct udp_demo_appstate uip_udp_appstate_t;


extern u8 udp_server_databuf[200];   	//�������ݻ���	  
extern u8 udp_server_sta;				//�����״̬


//����Ӧ�ó���ص����� 
#ifndef UIP_UDP_APPCALL
#define UIP_UDP_APPCALL udp_demo_appcall //����ص�����Ϊ udp_demo_appcall 
#endif


#endif
























