#include "udp_server.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "usart.h"
#include "string.h"
#include "lwip_demo.h"

struct udp_pcb* udp_server_pcb;//定义一个UDP的协议控制块
struct pbuf * ubuf;




//接收到数据包将要调用的函数
void udp_server_rev(void* arg,struct udp_pcb* upcb,struct pbuf* p,struct ip_addr*addr ,u16_t port){
	
	if(p!=NULL){
			if((p->tot_len)>=LWIP_DEMO_BUF){         //如果收的的数据大于缓存
				((char*)p->payload)[199] = 0;	         
				memcpy(lwip_demo_buf,p->payload,200);
			}else{				
				memcpy(lwip_demo_buf,p->payload,p->tot_len);
				lwip_demo_buf[p->tot_len] = 0;					
			}
			lwip_flag |= LWIP_NEW_DATA;		//收到了新的数据
			udp_server_pcb->remote_ip = *addr; //记录远程主机的IP和端口号
			udp_server_pcb->remote_port = port;
			pbuf_free(p);
  } 

}

//发送数据
void udp_server_send_data(void){
	err_t err;
	if((lwip_flag&LWIP_SEND_DATA)==LWIP_SEND_DATA){
    ubuf = pbuf_alloc(PBUF_TRANSPORT, strlen((char *)lwip_demo_buf), PBUF_RAM); 
		ubuf->payload = lwip_demo_buf;
		err=udp_send(udp_server_pcb,ubuf);//发送数据
		if(err!=ERR_OK){
			lwip_log("UDP SERVER发送数据失败！");
		}
	  lwip_flag &=~LWIP_SEND_DATA;		//清除发送数据的标志
		pbuf_free(ubuf);
	}
}

#define UDP_SERVER_PORT 1400


//初始化UDP服务器
void Init_UDP_Server(void){
	//struct ip_addr* ipaddr;
	 
	udp_server_pcb = udp_new();	//新建一个UDP协议控制块
  if(udp_server_pcb!=NULL){
		udp_bind(udp_server_pcb,IP_ADDR_ANY,UDP_SERVER_PORT);
		udp_recv(udp_server_pcb,udp_server_rev,NULL);				//指定收到数据包时的回调函数
	}
	
}
