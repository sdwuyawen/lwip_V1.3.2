#include "stm32f10x.h"
#include "bsp_ticktimer.h"

#include "lwipdemo.h"

#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/dhcp.h"
#include "lwip/tcp.h"	//#include "lwip/tcp_impl.h   for Version 1.4.1"
#include "lwip/ip_frag.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#include "netif/ethernetif.h"
#include "arch/sys_arch.h"

#include "netio.h"
#include "loopif.h"


/*******************************LwIP��ض���************************************************/

#define MS_PER_CLOCKTICK 10    //����LwIP��ʱ�ӽ���

static struct ip_addr ipaddr, netmask, gw; //����IP��ַ
struct netif enc28j60_netif;  //��������ӿ�
struct netif loop_netif;	//���廷�ؽӿ�
u32_t last_arp_time;			
u32_t last_tcp_time;	
u32_t last_ipreass_time;

u32_t input_time;

#if LWIP_DHCP>0	
u32_t last_dhcp_fine_time;			
u32_t last_dhcp_coarse_time;  
u32 dhcp_ip=0;
#endif


void loopclient_init(void);

//LWIP��ѯ
void LWIP_Polling(void)
{
//	if(timer_expired(&input_time,50 / 10)) //���հ������ڴ�����,50ms
	{
		/* ethernetif_input()���ýӿڵײ����뺯����������������ݰ��������
		 * �ӿڵ�input()���������ύ���ݰ�
		 * ethernetif_input() -> ethernet_input()
		 */
		ethernetif_input(&enc28j60_netif);	//��ѯ��̫���ӿ�
		netif_poll(&loop_netif);						//��ѯ���ؽӿ�
	}
	if(timer_expired(&last_tcp_time,TCP_TMR_INTERVAL/MS_PER_CLOCKTICK))		//TCP����ʱ��������
	{
		tcp_tmr();
	}
	if(timer_expired(&last_arp_time,ARP_TMR_INTERVAL/MS_PER_CLOCKTICK))		//ARP����ʱ��
	{
		etharp_tmr();
	}
	if(timer_expired(&last_ipreass_time,IP_TMR_INTERVAL/MS_PER_CLOCKTICK))//IP������װ��ʱ��
	{ 
		ip_reass_tmr();
	}
#if LWIP_DHCP>0			   					
	if(timer_expired(&last_dhcp_fine_time,DHCP_FINE_TIMER_MSECS/MS_PER_CLOCKTICK))
	{
		dhcp_fine_tmr();
	}
	if(timer_expired(&last_dhcp_coarse_time,DHCP_COARSE_TIMER_MSECS/MS_PER_CLOCKTICK))
	{
		dhcp_coarse_tmr();
	}  
#endif
	
	
	
}


/*
//��ʾ����״̬
void show_connect_status(void)
{
	if((lwip_flag&LWIP_CONNECTED)==LWIP_CONNECTED)
	{		//���ӳɹ���
		switch(lwip_test_mode)
		{
			case LWIP_TCP_SERVER:
				LCD_ShowString(30,180,200,16,16,"TCP Server Connected   ");
				break;
			case LWIP_TCP_CLIENT:
				LCD_ShowString(30,180,200,16,16,"TCP Client Connected   ");
				break;
			case LWIP_UDP_SERVER:
				LCD_ShowString(30,180,200,16,16,"UDP Server Connected   ");
				break;
			case LWIP_UDP_CLIENT:
				LCD_ShowString(30,180,200,16,16,"UDP Client Connected   ");
				break;					
			default:break;
		}
	}
	else
	{
		switch(lwip_test_mode)
		{
			case LWIP_TCP_SERVER:
				LCD_ShowString(30,180,200,16,16,"TCP Server Disconnected   ");
				break;
			case LWIP_TCP_CLIENT:
				LCD_ShowString(30,180,200,16,16,"TCP Client Disconnected   ");
				break;
			case LWIP_UDP_SERVER:
				LCD_ShowString(30,180,200,16,16,"UDP Server Connected   ");
				break;
			case LWIP_UDP_CLIENT:
				LCD_ShowString(30,180,200,16,16,"UDP Client Connected   ");
				break;	
			case LWIP_WEBSERVER:
				LCD_ShowString(30,180,200,16,16,"Webserver   "); 
				break;
			default:break;
		}
	}
}

//ѡ��ģʽ
void select_lwip_mode(void){
	u16 times=0;
	u8 key=0;
	u8 mode=0;
	u8 flag=0;
	POINT_COLOR = BLUE;
	do{
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{	
			mode++;
			switch(mode%5)
			{
				case 0: 
					lwip_test_mode = LWIP_TCP_SERVER;
					break;				
				case 1: 
					lwip_test_mode = LWIP_TCP_CLIENT;
					break;
				case 2: 
					lwip_test_mode = LWIP_UDP_SERVER;
					break;		
				case 3: 
					lwip_test_mode = LWIP_UDP_CLIENT;
					break;	
				case 4: 
					lwip_test_mode = LWIP_WEBSERVER;
					break;					
				default:break;
			}
		}
		bsp_DelayMS(1);
		times++;
		if((times%500==0))
		{
			if(!flag)
			{
				show_connect_status();
			}
			else
			{
				LCD_ShowString(30,180,200,16,16,"                              ");   
			}
			flag = !flag;
		}
		
	}
	while(key!=KEY_DOWN);

	show_connect_status();
}
struct ip_addr DNS_Addr;	 
//
void dns_serverFound(const char *name, struct ip_addr *ipaddr, void *arg)
{
	u32 ip=0;
	if ((ipaddr) && (ipaddr->addr))
	{
		ip = ipaddr->addr;
		printf("IP��ַ��%ld,%ld,%ld,%ld\r\n",(ip&0x000000ff),(ip&0x0000ff00)>>8,(ip&0x00ff0000)>>16,(ip&0xff000000)>>24);	
	}
	else
	{
		
	}
	
}

*/


static err_t tcpserver_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{

	//  char *rq;

	/* We perform here any necessary processing on the pbuf */
	if (p != NULL) 
	{        
		/* We call this function to tell the LwIp that we have processed the data */
		/* This lets the stack advertise a larger window, so more data can be received*/
		/* ֪ͨЭ��ջ���½��մ��� */
		tcp_recved(pcb, p->tot_len);

		{
			// Uart_Printf("Do tcp write when receive\n");
			/* �ѽ��յ��ĵ�һ��pbuf�е����ݷ��ظ��ͻ��� */
			tcp_write(pcb, p->payload, p->len, 1);
		}
		/* �ͷ�pbuf */
		pbuf_free(p);
	}
	/* ���pbuf == NULL �� err == ERR_OK����ʾ��һ���ڹرո�TCP���� */
	else if (err == ERR_OK) 
	{
		/* When the pbuf is NULL and the err is ERR_OK, the remote end is closing the connection. */
		/* We free the allocated memory and we close the connection */
		return tcp_close(pcb);
	}
	return ERR_OK;
}

/**
  * @brief  This function when the Telnet connection is established
  * @param  arg  user supplied argument 
  * @param  pcb	 the tcp_pcb which accepted the connection
  * @param  err	 error value
  * @retval ERR_OK
  */

static err_t tcpserver_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{     
  
	/* Tell LwIP to associate this structure with this connection. */
	// tcp_arg(pcb, mem_calloc(sizeof(struct name), 1));	

	/* Configure LwIP to use our call back functions. */
	// tcp_err(pcb, HelloWorld_conn_err);
	// tcp_setprio(pcb, TCP_PRIO_MIN);
	tcp_recv(pcb, tcpserver_recv);
	// tcp_poll(pcb, http_poll, 10);
	//  tcp_sent(pcb, http_sent);
	/* Send out the first message */  
	// tcp_write(pcb, GREETING, strlen(GREETING), 1); 
	return ERR_OK;
}

/**
  * @brief  Initialize the hello application  
  * @param  None 
  * @retval None 
  */
 
void tcpserver_init(void)
{
	struct tcp_pcb *pcb;	            		

	/* Create a new TCP control block  */
	pcb = tcp_new();	                		 	

	/* Assign to the new pcb a local IP address and a port number */
	/* Using IP_ADDR_ANY allow the pcb to be used by any local interface */
	tcp_bind(pcb, IP_ADDR_ANY, 6060);       


	/* Set the connection to the LISTEN state */
	pcb = tcp_listen(pcb);				

	/* Specify the function to be called when a connection is established */	
	tcp_accept(pcb, tcpserver_accept);   
										
}



static err_t loopclient_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{

//  char *rq;

  /* We perform here any necessary processing on the pbuf */
  if (p != NULL) 
  {        
	/* We call this function to tell the LwIp that we have processed the data */
	/* This lets the stack advertise a larger window, so more data can be received*/
	char *data = (char *)p->payload;
	int length = p->len;
	tcp_recved(pcb, p->tot_len);

    printf("LoopClient Get Data::");
    while(length-- >0) 
	{
		printf("%c", (int)(*data++));
	}
		
//    OSTimeDly(50);
//    tcp_write(pcb, p->payload, p->len, 1);
    pbuf_free(p);
   } 
  else if (err == ERR_OK) 
  {
    return tcp_close(pcb);
  }
  return ERR_OK;
}
unsigned char loopdata[]="Loop Interface Test!!\n";
static err_t loopclient_connect(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  printf("TCP loop connected\n");
  tcp_recv(tpcb, loopclient_recv);
//  OSTimeDly(10);
  tcp_write(tpcb,loopdata,sizeof(loopdata)-1, 1);
  return ERR_OK;
}

void loopclient_init(void)
{
  struct tcp_pcb *pcb;
  struct ip_addr ipaddr;
  IP4_ADDR(&ipaddr, 127,0,0,1); 
  /* Create a new TCP control block  */
  pcb = tcp_new();	                		 	

  /* Assign to the new pcb a local IP address and a port number */
  /* Using IP_ADDR_ANY allow the pcb to be used by any local interface */
  tcp_bind(pcb, IP_ADDR_ANY, 7);       

  tcp_connect(pcb,&ipaddr,6060,loopclient_connect);
}

// err_t loopif_init(struct netif *netif)
// {
//   /* initialize the snmp variables and counters inside the struct netif
//    * ifSpeed: no assumption can be made!
//    */
//   NETIF_INIT_SNMP(netif, snmp_ifType_softwareLoopback, 0);

//   netif->name[0] = 'l';
//   netif->name[1] = 'o';
//   netif->output = netif_loop_output;
//   return ERR_OK;
// }

void LwIP_APP_Init(void)
{
	/*LwIP��ʼ����ʼ*/
	#if LWIP_DHCP > 0			   					//��ʹ��DHCPЭ��
	  ipaddr.addr = 0;
	  netmask.addr = 0;
	  gw.addr = 0; 
	#else										//
	  IP4_ADDR(&ipaddr, 202, 194, 201, 252);  		//���ñ���ip��ַ
	  IP4_ADDR(&gw, 202, 194, 201, 254);			//����
	  IP4_ADDR(&netmask, 255, 255, 255, 0);		//��������	 
	#endif
 
	init_lwip_timer();  //��ʼ��LWIP��ʱ��
	//��ʼ��LWIPЭ��ջ,ִ�м���û����п����õ�ֵ����ʼ�����е�ģ��
	lwip_init(); 
	
	//�������ӿ�enc28j60
	while((netif_add(&enc28j60_netif, 
										&ipaddr, 
										&netmask, 
										&gw, 
										NULL, 
										&ethernetif_init, 				/* err_t (* init)(struct netif *netif) */
										&ethernet_input					/* err_t (* input)(struct pbuf *p, struct netif *netif)) ARP������ݰ����뺯�� */
									) == NULL))
	{
		LCD_ShowString(60,170,200,16,16,"ENC28J60 Init Error!");	 
		bsp_DelayMS(200);
		LCD_Fill(60,170,240,186,WHITE);//���֮ǰ��ʾ
		bsp_DelayMS(200);
	}
	//ע��Ĭ�ϵ�����ӿ�
	netif_set_default(&enc28j60_netif);
	//ʹ�ܸ�����ӿ�
	netif_set_up(&enc28j60_netif); 
	
	//��ӻ��ؽӿ�loopif
	IP4_ADDR(&gw, 127,0,0,1);
	IP4_ADDR(&ipaddr, 127,0,0,1);
	IP4_ADDR(&netmask, 255,255,255,0);

	netif_add(&loop_netif,
						&ipaddr, 
						&netmask, 
						&gw, 
						NULL, 
						loopif_init,NULL);
	netif_set_up(&loop_netif);
	 
	/*
	Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
	the predefined regular intervals after starting the client.
	You can peek in the netif->dhcp struct for the actual DHCP status.	
	ip_addr_t offered_ip_addr;
	ip_addr_t offered_sn_mask;
	ip_addr_t offered_gw_addr;
		dhcp_ip = enc28j60_netif.dhcp->offered_ip_addr.addr;
		printf("IP��ַ��%ld,%ld,%ld,%ld\t\n",(dhcp_ip&0x000000ff),(dhcp_ip&0x0000ff00)>>8,(dhcp_ip&0x00ff0000)>>16,(dhcp_ip&0xff000000)>>24);	
	*/
	#if LWIP_DHCP > 0			   					//��ʹ��DHCPЭ��
	dhcp_start(&enc28j60_netif);             //Ϊ��������һ���µ�DHCP�ͻ���
	#endif

// 	select_lwip_mode();  //ѡ����Ҫ���е�ʵ��
// 	lwip_demo_init();		//��ʼ��lwip_demo

	tcpserver_init();	//��ʼ��TCP������
	
	netio_init();		//��ʼ��NETIO
	
	loopclient_init();	//��ʼ�����ؽӿ�  client
}
