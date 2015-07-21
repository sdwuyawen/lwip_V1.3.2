#include "tcp_client.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "usart.h"
#include "string.h"
//#include "lwip_demo.h"

struct tcp_pcb* tcp_client_pcb;//����һ��TCP��Э����ƿ�
static const char* respond =  "Compass STM32 Board Connected Successfully!\r\n";



enum tcp_client_states			//����״̬
{
  ES_NONE = 0,			
  ES_RECEIVED,		//���յ�������
  ES_CLOSING			//���ӹر�
};

struct tcp_client_state //TCP������״̬
{
  u8_t state;

};


err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb);//������ѯʱ��Ҫ���õĺ���
err_t tcp_client_connect(void *arg,struct tcp_pcb *tpcb,err_t err);//���ӳɹ�ʱ��Ҫ���õĺ���
void tcp_client_close(struct tcp_pcb *tpcb, struct tcp_client_state* ts);	//�ر�����

//�ͻ��˳ɹ����ӵ�Զ������ʱ����
err_t tcp_client_connect(void *arg,struct tcp_pcb *tpcb,err_t err){
		struct tcp_client_state* ts;
	  ts = arg;	
	  ts->state =   ES_RECEIVED;		//���Կ�ʼ����������
	  lwip_flag |= LWIP_CONNECTED;		//������ӳɹ���
		tcp_write(tpcb,respond,strlen(respond),1);  //��Ӧ��Ϣ	
		return ERR_OK;
}


//������ѯʱ��Ҫ���õĺ���
err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb){
		err_t ret_err;
		struct tcp_client_state* ts;
	  ts = arg;
		lwip_log("tcp_client_polling!\r\n");
		if(ts!=NULL){		//���Ӵ��ڿ��п��Է�������
				if((lwip_flag&LWIP_SEND_DATA)==LWIP_SEND_DATA){
						tcp_write(tpcb,lwip_demo_buf,strlen((char *)lwip_demo_buf),1);//��������
					  lwip_flag &=~LWIP_SEND_DATA;		//����������ݵı�־
				}
		}else{
			tcp_abort(tpcb);
			ret_err = ERR_ABRT;
		}
		return ret_err;
}

//��������Զ������
void tcp_client_connect_remotehost(void){
	Init_TCP_Client();
}


//�ͻ��˽��յ�����֮��Ҫ���õĺ���
err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
		err_t ret_err;
		struct tcp_client_state* ts;
		ts = arg;			//TCP PCB״̬
		if(p==NULL){	
				ts->state = ES_CLOSING;		//���ӹر���
				tcp_client_close(tpcb,ts);
				lwip_flag &=~ LWIP_CONNECTED;	//������ӱ�־
			  printf("���ӹر��ˣ�1\r\n");
			
		}else if(err!=ERR_OK){	//λ�ô����ͷ�pbuf
				if(p!=NULL){
					pbuf_free(p);
				}
				ret_err = err;		//�õ�����
		}else if(ts->state==ES_RECEIVED){//�����յ����µ�����

			//	printf("�������½��յ�����:%s\r\n",p->payload);
			  if((p->tot_len)>=LWIP_DEMO_BUF){         //����յĵ����ݴ��ڻ���
					((char*)p->payload)[199] = 0;	         
					memcpy(lwip_demo_buf,p->payload,200);
				}else{				
					memcpy(lwip_demo_buf,p->payload,p->tot_len);
					lwip_demo_buf[p->tot_len] = 0;					
				}

				
				
				lwip_flag |= LWIP_NEW_DATA;		//�յ����µ�����
			  tcp_recved(tpcb, p->tot_len);		//���ڻ�ȡ�������ݵĳ���,	��ʾ���Ի�ȡ���������			
				pbuf_free(p);	//�ͷ��ڴ�
				ret_err = ERR_OK;
		}else if(ts->state==ES_CLOSING){	//�������ر���
				tcp_recved(tpcb, p->tot_len);		//Զ�̶˿ڹر����Σ���������
				pbuf_free(p);
			  ret_err = ERR_OK;
			  printf("���ӹر��ˣ�2\r\n");
		}else{										//����δ֪״̬
				tcp_recved(tpcb, p->tot_len);
				pbuf_free(p);
			  ret_err = ERR_OK;
		}
		return ret_err;
	
}


//�ر�����
void tcp_client_close(struct tcp_pcb *tpcb, struct tcp_client_state* ts){

	tcp_arg(tcp_client_pcb, NULL);  			
	tcp_recv(tcp_client_pcb, NULL);
	tcp_poll(tcp_client_pcb, NULL, 0); 
	if(ts!=NULL){
		mem_free(ts);
	}
	tcp_close(tpcb);
}

#define TCP_CLIENT_PORT  1300
//��ʼ��TCP�ͻ���
void Init_TCP_Client(void){
	struct tcp_client_state* ts;
	ip_addr_t ipaddr;
	IP4_ADDR(&ipaddr, 192, 168, 1, 100);  	
	
	tcp_client_pcb = tcp_new();				//�½�һ��PCB
	if(tcp_client_pcb!=NULL){
		
		  ts = mem_malloc(sizeof(struct tcp_client_state));	 //�����ڴ�
		  tcp_arg(tcp_client_pcb, ts);  				//�������Э����ƿ��״̬���ݸ����еĻص�����
		
      tcp_connect(tcp_client_pcb,&ipaddr,TCP_CLIENT_PORT,tcp_client_connect);
		  tcp_recv(tcp_client_pcb, tcp_client_recv);	//ָ�����ӽ��յ��µ�����֮��Ҫ���õĻص�����
			tcp_poll(tcp_client_pcb, tcp_client_poll, 0); //ָ����ѯʱ��Ҫ���õĻص�����

	}
}


