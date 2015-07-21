#include "tcp_server.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "usart.h"
#include "string.h"
#include "lwip_demo.h"




struct tcp_pcb* tcp_server_pcb;//����һ��TCP��Э����ƿ�
static const char* respond =  "ALIENTEK STM32 Board Connected Successfully!\r\n";



enum tcp_server_states			//����״̬
{
  ES_NONE = 0,			
  ES_RECEIVED,		//���յ�������
  ES_CLOSING			//���ӹر�
};

struct tcp_server_state //TCP������״̬
{
  u8_t state;

};



err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);  //���������ӳɹ���Ҫ���õĺ���
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);	//���������յ�����֮��Ҫ���õĺ���
err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb);//������ѯʱ��Ҫ���õĺ���
void tcp_server_error(void *arg,err_t err);	//���ӳ���Ҫ���õĻص�����
void tcp_server_close(struct tcp_pcb *tpcb, struct tcp_server_state* ts); //�ر�����


#define TCP_SERVER_PORT  1200  //����TCP�������˿�
//��ʼ��LWIP������
void Init_TCP_Server(void){
		err_t err;				//LWIP������Ϣ
		tcp_server_pcb = tcp_new();		//�½�һ��TCPЭ����ƿ�
		if(tcp_server_pcb!=NULL){
				err = tcp_bind(tcp_server_pcb,IP_ADDR_ANY,TCP_SERVER_PORT);//�󶨱�������IP��ַ�Ͷ˿ں� ��Ϊ����������Ҫ֪���ͻ��˵�IP
				if(err==ERR_OK){//�ɹ���
					tcp_server_pcb = tcp_listen(tcp_server_pcb);	//��ʼ�����˿�
					tcp_accept(tcp_server_pcb,tcp_server_accept); //ָ������״̬��������֮ͨ��Ҫ���õĻص�����
				}
		}
	

}

//���������ӳɹ���Ҫ���õĺ���
err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err){
		err_t ret_err;
		struct tcp_server_state* ts;
		
	  ts = mem_malloc(sizeof(struct tcp_server_state));	 //�����ڴ�
		if(ts!=NULL){
				ts->state = ES_RECEIVED;							//���Խ���������
			  lwip_flag |= LWIP_CONNECTED;				//�Ѿ���������
			  tcp_write(newpcb,respond,strlen(respond),1);  //��Ӧ��Ϣ
			
				tcp_arg(newpcb, ts);  				//�������Э����ƿ��״̬���ݸ����еĻص�����

				tcp_recv(newpcb, tcp_server_recv);	//ָ�����ӽ��յ��µ�����֮��Ҫ���õĻص�����
				tcp_err(newpcb, tcp_server_error);	//ָ�����ӳ���Ҫ���õĺ���
				tcp_poll(newpcb, tcp_server_poll, 0); //ָ����ѯʱ��Ҫ���õĻص�����
				ret_err = ERR_OK;

		}else{
				ret_err = ERR_MEM;
		}
		return ret_err;
	
}

//������ѯʱ��Ҫ���õĺ���
err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb){
		err_t ret_err;
		struct tcp_server_state* ts;
	  ts = arg;
		lwip_log("tcp_server_polling!\r\n");
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

//���������յ�����֮��Ҫ���õĺ���
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
		err_t ret_err;
		struct tcp_server_state* ts;
		
		ts = arg;			//TCP PCB״̬
		if(p==NULL){	
				ts->state = ES_CLOSING;		//���ӹر���
				tcp_server_close(tpcb,ts);
				lwip_flag &=~ LWIP_CONNECTED;	//������ӱ�־
			
		}else if(err!=ERR_OK){	//δ֪����,�ͷ�pbuf
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
				
				tcp_recved(tpcb, p->tot_len);		//���ڻ�ȡ�������ݵĳ���,	֪ͨLWIP�Ѿ���ȡ�����ݣ����Ի�ȡ���������
				pbuf_free(p);	//�ͷ��ڴ�
				ret_err = ERR_OK;
		}else if(ts->state==ES_CLOSING){	//�������ر���
				tcp_recved(tpcb, p->tot_len);		//Զ�̶˿ڹر����Σ���������
				pbuf_free(p);
			  ret_err = ERR_OK;
		}else{										//����δ֪״̬
				tcp_recved(tpcb, p->tot_len);
				pbuf_free(p);
			  ret_err = ERR_OK;
		}
		return ret_err;
	
}

//���ӳ���Ҫ���õĺ���
void tcp_server_error(void *arg,err_t err){
		struct tcp_server_state* ts;
	  ts = arg;
	  if(ts!=NULL){
			mem_free(ts);
		}
}

//�ر�����
void tcp_server_close(struct tcp_pcb *tpcb, struct tcp_server_state* ts){

	if(ts!=NULL){
		mem_free(ts);
	}
	tcp_close(tpcb);
}




