#include "tcp_server.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "usart.h"
#include "string.h"
#include "lwip_demo.h"




struct tcp_pcb* tcp_server_pcb;//定义一个TCP的协议控制块
static const char* respond =  "ALIENTEK STM32 Board Connected Successfully!\r\n";



enum tcp_server_states			//连接状态
{
  ES_NONE = 0,			
  ES_RECEIVED,		//接收到了数据
  ES_CLOSING			//连接关闭
};

struct tcp_server_state //TCP服务器状态
{
  u8_t state;

};



err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);  //服务器连接成功后将要调用的函数
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);	//服务器接收到数据之后将要调用的函数
err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb);//连接轮询时将要调用的函数
void tcp_server_error(void *arg,err_t err);	//连接出错将要调用的回调函数
void tcp_server_close(struct tcp_pcb *tpcb, struct tcp_server_state* ts); //关闭连接


#define TCP_SERVER_PORT  1200  //定义TCP服务器端口
//初始化LWIP服务器
void Init_TCP_Server(void){
		err_t err;				//LWIP错误信息
		tcp_server_pcb = tcp_new();		//新建一个TCP协议控制块
		if(tcp_server_pcb!=NULL){
				err = tcp_bind(tcp_server_pcb,IP_ADDR_ANY,TCP_SERVER_PORT);//绑定本地所有IP地址和端口号 作为服务器不需要知道客户端的IP
				if(err==ERR_OK){//成功绑定
					tcp_server_pcb = tcp_listen(tcp_server_pcb);	//开始监听端口
					tcp_accept(tcp_server_pcb,tcp_server_accept); //指定监听状态的连接联通之后将要调用的回调函数
				}
		}
	

}

//服务器连接成功后将要调用的函数
err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err){
		err_t ret_err;
		struct tcp_server_state* ts;
		
	  ts = mem_malloc(sizeof(struct tcp_server_state));	 //申请内存
		if(ts!=NULL){
				ts->state = ES_RECEIVED;							//可以接收数据了
			  lwip_flag |= LWIP_CONNECTED;				//已经连接上了
			  tcp_write(newpcb,respond,strlen(respond),1);  //回应信息
			
				tcp_arg(newpcb, ts);  				//将程序的协议控制块的状态传递给多有的回调函数

				tcp_recv(newpcb, tcp_server_recv);	//指定连接接收到新的数据之后将要调用的回调函数
				tcp_err(newpcb, tcp_server_error);	//指定连接出错将要调用的函数
				tcp_poll(newpcb, tcp_server_poll, 0); //指定轮询时将要调用的回调函数
				ret_err = ERR_OK;

		}else{
				ret_err = ERR_MEM;
		}
		return ret_err;
	
}

//连接轮询时将要调用的函数
err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb){
		err_t ret_err;
		struct tcp_server_state* ts;
	  ts = arg;
		lwip_log("tcp_server_polling!\r\n");
		if(ts!=NULL){		//连接处于空闲可以发送数据
				if((lwip_flag&LWIP_SEND_DATA)==LWIP_SEND_DATA){
						tcp_write(tpcb,lwip_demo_buf,strlen((char *)lwip_demo_buf),1);//发送数据
					  lwip_flag &=~LWIP_SEND_DATA;		//清除发送数据的标志
				}
		}else{
			tcp_abort(tpcb);
			ret_err = ERR_ABRT;
		}
		return ret_err;
}

//服务器接收到数据之后将要调用的函数
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
		err_t ret_err;
		struct tcp_server_state* ts;
		
		ts = arg;			//TCP PCB状态
		if(p==NULL){	
				ts->state = ES_CLOSING;		//连接关闭了
				tcp_server_close(tpcb,ts);
				lwip_flag &=~ LWIP_CONNECTED;	//清除连接标志
			
		}else if(err!=ERR_OK){	//未知错误,释放pbuf
				if(p!=NULL){
					pbuf_free(p);
				}
				ret_err = err;		//得到错误
		}else if(ts->state==ES_RECEIVED){//连接收到了新的数据

			//	printf("服务器新接收的数据:%s\r\n",p->payload);

			  if((p->tot_len)>=LWIP_DEMO_BUF){         //如果收的的数据大于缓存
					((char*)p->payload)[199] = 0;	         
					memcpy(lwip_demo_buf,p->payload,200);
				}else{				
					memcpy(lwip_demo_buf,p->payload,p->tot_len);
					lwip_demo_buf[p->tot_len] = 0;					
				}
				
				lwip_flag |= LWIP_NEW_DATA;		//收到了新的数据
				
				tcp_recved(tpcb, p->tot_len);		//用于获取接收数据的长度,	通知LWIP已经读取了数据，可以获取更多的数据
				pbuf_free(p);	//释放内存
				ret_err = ERR_OK;
		}else if(ts->state==ES_CLOSING){	//服务器关闭了
				tcp_recved(tpcb, p->tot_len);		//远程端口关闭两次，垃圾数据
				pbuf_free(p);
			  ret_err = ERR_OK;
		}else{										//其他未知状态
				tcp_recved(tpcb, p->tot_len);
				pbuf_free(p);
			  ret_err = ERR_OK;
		}
		return ret_err;
	
}

//连接出错将要调用的函数
void tcp_server_error(void *arg,err_t err){
		struct tcp_server_state* ts;
	  ts = arg;
	  if(ts!=NULL){
			mem_free(ts);
		}
}

//关闭连接
void tcp_server_close(struct tcp_pcb *tpcb, struct tcp_server_state* ts){

	if(ts!=NULL){
		mem_free(ts);
	}
	tcp_close(tpcb);
}




