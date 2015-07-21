#ifndef  __LWIP_DEMO_H
#define  __LWIP_DEMO_H

//#include "mysys.h"
//#include "tcp_server.h"
//#include "tcp_client.h"
//#include "udp_server.h"
//#include "udp_client.h"


#define LWIP_CONNECTED  0X80 //���ӳɹ�
#define LWIP_NEW_DATA   0x40 //���µ�����
#define LWIP_SEND_DATA  0x20 //��������Ҫ����
#define LWIP_DEMO_BUF        200

#define LWIP_TCP_SERVER 0x80  //tcp ����������
#define LWIP_TCP_CLIENT 0x40	//tcp �ͻ��˹���
#define LWIP_UDP_SERVER 0x20  //UDP ����������
#define LWIP_UDP_CLIENT 0x10  //UDP �ͻ��˹���
#define LWIP_WEBSERVER  0x08  //UDP �ͻ��˹���


#define LWIP_DEMO_DEBUG 1  //�Ƿ��ӡ������Ϣ   

extern u8 lwip_flag;


extern u8 lwip_test_mode;
extern u8 lwip_demo_buf[];

void lwip_demo_init(void);	//��ʼ��LWIP_DEMO
void lwip_log(char *str);  //��ӡӦ�ó���״̬

#endif

