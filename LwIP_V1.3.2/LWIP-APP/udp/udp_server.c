#include "udp_server.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "usart.h"
#include "string.h"
#include "lwip_demo.h"

struct udp_pcb* udp_server_pcb;//����һ��UDP��Э����ƿ�
struct pbuf * ubuf;




//���յ����ݰ���Ҫ���õĺ���
void udp_server_rev(void* arg,struct udp_pcb* upcb,struct pbuf* p,struct ip_addr*addr ,u16_t port){
	
	if(p!=NULL){
			if((p->tot_len)>=LWIP_DEMO_BUF){         //����յĵ����ݴ��ڻ���
				((char*)p->payload)[199] = 0;	         
				memcpy(lwip_demo_buf,p->payload,200);
			}else{				
				memcpy(lwip_demo_buf,p->payload,p->tot_len);
				lwip_demo_buf[p->tot_len] = 0;					
			}
			lwip_flag |= LWIP_NEW_DATA;		//�յ����µ�����
			udp_server_pcb->remote_ip = *addr; //��¼Զ��������IP�Ͷ˿ں�
			udp_server_pcb->remote_port = port;
			pbuf_free(p);
  } 

}

//��������
void udp_server_send_data(void){
	err_t err;
	if((lwip_flag&LWIP_SEND_DATA)==LWIP_SEND_DATA){
    ubuf = pbuf_alloc(PBUF_TRANSPORT, strlen((char *)lwip_demo_buf), PBUF_RAM); 
		ubuf->payload = lwip_demo_buf;
		err=udp_send(udp_server_pcb,ubuf);//��������
		if(err!=ERR_OK){
			lwip_log("UDP SERVER��������ʧ�ܣ�");
		}
	  lwip_flag &=~LWIP_SEND_DATA;		//����������ݵı�־
		pbuf_free(ubuf);
	}
}

#define UDP_SERVER_PORT 1400


//��ʼ��UDP������
void Init_UDP_Server(void){
	//struct ip_addr* ipaddr;
	 
	udp_server_pcb = udp_new();	//�½�һ��UDPЭ����ƿ�
  if(udp_server_pcb!=NULL){
		udp_bind(udp_server_pcb,IP_ADDR_ANY,UDP_SERVER_PORT);
		udp_recv(udp_server_pcb,udp_server_rev,NULL);				//ָ���յ����ݰ�ʱ�Ļص�����
	}
	
}
