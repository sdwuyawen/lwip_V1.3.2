#include "udp_client.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "usart.h"
#include "string.h"
#include "lwip_demo.h"

struct udp_pcb* udp_client_pcb;//定义一个UDP的协议控制块
struct pbuf * ubuf_client;




//接收到数据包将要调用的函数
void udp_client_rev(void* arg,struct udp_pcb* upcb,struct pbuf* p,struct ip_addr*addr ,u16_t port){
	
	if(p!=NULL){
			if((p->tot_len)>=LWIP_DEMO_BUF){         //如果收的的数据大于缓存
				((char*)p->payload)[199] = 0;	         
				memcpy(lwip_demo_buf,p->payload,200);
			}else{				
				memcpy(lwip_demo_buf,p->payload,p->tot_len);
				lwip_demo_buf[p->tot_len] = 0;					
			}
			lwip_flag |= LWIP_NEW_DATA;		//收到了新的数据
			pbuf_free(p);
  } 

}

//发送数据
void udp_client_send_data(void){
	err_t err;
	if((lwip_flag&LWIP_SEND_DATA)==LWIP_SEND_DATA){
    ubuf_client = pbuf_alloc(PBUF_TRANSPORT, strlen((char *)lwip_demo_buf), PBUF_RAM); //为发送包分配内存
		ubuf_client->payload = lwip_demo_buf;
		err=udp_send(udp_client_pcb,ubuf_client);//发送数据
		if(err!=ERR_OK){
			lwip_log("UDP SERVER发送数据失败！");
		}
	  lwip_flag &=~LWIP_SEND_DATA;		//清除发送数据的标志
		pbuf_free(ubuf_client);
	}
}

#define UDP_CLIENT_PORT 1500

//初始化UDP客户端
void Init_UDP_Client(void){
	//struct ip_addr* ipaddr;
	ip_addr_t ipaddr;
	IP4_ADDR(&ipaddr, 192, 168, 1, 100);  		//设置本地ip地址
	udp_client_pcb = udp_new();	//新建一个UDP协议控制块
  if(udp_client_pcb!=NULL){
		udp_bind(udp_client_pcb,IP_ADDR_ANY,UDP_CLIENT_PORT);		//
    udp_connect(udp_client_pcb,&ipaddr,UDP_CLIENT_PORT);  //设置连接到远程主机
		udp_recv(udp_client_pcb,udp_client_rev,NULL);				//指定收到数据包时的回调函数
	}
	
}
