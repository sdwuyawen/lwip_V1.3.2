#include "stm32f10x.h"
#include "bsp_ticktimer.h"
#include "string.h"

#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/dhcp.h"
#include "lwip/tcp.h"	//#include "lwip/tcp_impl.h   for Version 1.4.1"
#include "lwip/ip_frag.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#include "netif/ethernetif.h"
#include "arch/sys_arch.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

#include "netio.h"
#include "loopif.h"


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Called when a data is received on the telnet connection
  * @param  arg	the user argument
  * @param  pcb	the tcp_pcb that has received the data
  * @param  p	the packet buffer
  * @param  err	the error value linked with the received data
  * @retval error value
  */
char sndbuf[50], cmdbuf[20];
s8_t cmd_flag;

static err_t telnet_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
//  char *rq;

  /* We perform here any necessary processing on the pbuf */
  if (p != NULL) 
  {  
	 u16_t len;
	 u8_t *datab;
	 u16_t strlen;      
	/* We call this function to tell the LwIp that we have processed the data */
	/* This lets the stack advertise a larger window, so more data can be received*/
	tcp_recved(pcb, p->tot_len);
	//do
	{
		//data=p->payload;
		len=p->len;
		datab = (unsigned char *)p->payload;
		if((len == 2) && (*datab == 0x0d) && (*(datab+1) == 0x0a))
		{	
			if(cmd_flag > 0)
			{
			  cmdbuf[cmd_flag] = 0x00;
			  if(strcmp(cmdbuf,"date")==0)
			  {
				strlen = sprintf(sndbuf, "Now, It is 2011-xx-xx!\r\n");
				tcp_write(pcb,sndbuf,strlen, 1);
			  }
			  else if(strcmp(cmdbuf, "hello")==0)
			  {
				strlen = sprintf(sndbuf, "Hello, Nice to see you!\r\n");
				tcp_write(pcb,sndbuf,strlen, 1);
			  }
			  else if(strcmp(cmdbuf, "more")==0)
			  {
				strlen = sprintf(sndbuf, "Add whatever you need in this way!\r\n");
				tcp_write(pcb,sndbuf,strlen, 1);
			  }
			  else if(strcmp(cmdbuf, "help")==0)
			  {
				strlen = sprintf(sndbuf, "Suppprted Command：date  hello  more  help  quit\r\n");
				tcp_write(pcb,sndbuf,strlen, 1);
			  }
			  else if(strcmp(cmdbuf, "quit")==0)
			  {
				cmd_flag=0;
				pbuf_free(p);
				return tcp_close(pcb);
			  }
			 else
			  {
			   strlen = sprintf(sndbuf, "Unkonwn Command: %s.\r\n", cmdbuf);
			   tcp_write(pcb,sndbuf,strlen, 1);
			 }
			  cmd_flag = 0;
			}
			strlen = sprintf(sndbuf,"Forrest_Shell>>");
			tcp_write(pcb,sndbuf,strlen, 1);
		 }
		 else if((len == 1) && (*datab >= 0x20) && (*datab <= 0x7e) && (cmd_flag < 19))
		 {
			cmdbuf[cmd_flag] = *datab;
			cmd_flag++;
		 }
		else if((len == 1) && (*datab == 0x08) && (cmd_flag >0))
		{
			cmd_flag--;
			strlen = sprintf(sndbuf," \b \b");
			tcp_write(pcb,sndbuf,strlen, 1);
		}
		else if((len == 1) && (*datab == 0x08))
		{
			cmd_flag=0;
			strlen = sprintf(sndbuf,">");
			tcp_write(pcb,sndbuf,strlen, 1);
		}
		else
		{
					
		}
	}
//	while(netbuf_next(buf) >= 0);
     pbuf_free(p);
    }
	/* pbuf为NULL，表示收到了对方的FIN，关闭连接 */
  else if (err == ERR_OK) 
  {
    /* When the pbuf is NULL and the err is ERR_OK, the remote end is closing the connection. */
    /* We free the allocated memory and we close the connection */
    cmd_flag = 0;  //断开连接，清除标志
    return tcp_close(pcb);
  }
  return ERR_OK;
}

static err_t telnet_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{     
  u16_t strlen;
  tcp_recv(pcb, telnet_recv);
  
  strlen = sprintf(sndbuf,"##Welcome to demo TELNET based on LwIP##\r\n");
  tcp_write(pcb, sndbuf, strlen,TCP_WRITE_FLAG_COPY); 
  strlen = sprintf(sndbuf,"##Created by Forrest...               ##\r\n");
  tcp_write(pcb, sndbuf, strlen,TCP_WRITE_FLAG_COPY);
  strlen = sprintf(sndbuf,"##quit:退出    help:帮助信息          ##\r\n");
  tcp_write(pcb, sndbuf, strlen,TCP_WRITE_FLAG_COPY);
  strlen = sprintf(sndbuf,"Forrest_Shell>>");
  tcp_write(pcb,sndbuf,strlen, 1);

  return ERR_OK;
}

/**
  * @brief  Initialize the hello application  
  * @param  None 
  * @retval None 
  */
 
void telnet_init(void)
{
  struct tcp_pcb *pcb;	            		
  
  /* Create a new TCP control block  */
  pcb = tcp_new();	                		 	

  /* Assign to the new pcb a local IP address and a port number */
  /* Using IP_ADDR_ANY allow the pcb to be used by any local interface */
  tcp_bind(pcb, IP_ADDR_ANY, 23);       


  /* Set the connection to the LISTEN state */
  pcb = tcp_listen(pcb);				

  /* Specify the function to be called when a connection is established */	
  tcp_accept(pcb, telnet_accept);   
										
}

