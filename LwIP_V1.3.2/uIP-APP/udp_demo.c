#include "uip.h"	    
#include "udp_demo.h"
#include "string.h"
#include "stdio.h"
//////////////////////////////////////////////////////////////////////////////////	 
//ALIENTEK战舰STM32开发板
//uIP TCP测试 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/28
//版本：V1.0			   								  
//////////////////////////////////////////////////////////////////////////////////

u8 udp_server_databuf[200];   	//发送数据缓存	  
u8 udp_server_sta;				//服务端状态


void UDP_5555_APP(void);
void UDP_5566_APP(void);

//UDP应用接口函数(UIP_UDP_APPCALL)
void udp_demo_appcall(void)
{	
  	
	switch(uip_udp_conn->rport)
	{
		case HTONS(5555):
			UDP_5555_APP();
			break;
		case HTONS(5566):
			UDP_5566_APP();
			break;
		case HTONS(68):
//			dhcpc_appcall();
			break;
		case HTONS(53):
//			resolv_appcall();
		default:
			break;
	}		    
}


void UDP_5555_APP(void)
{
    /* 判断当前状态 */
    if (uip_poll())
    {
        char *tmp_dat = "the 5555 auto send!\r\n";
        uip_send((char *)tmp_dat,strlen(tmp_dat));
    }
    if (uip_newdata())
    {
        char *tmp_dat = "5555 receive the data!\r\n";
        /* 收到新数据 */
        printf("%d ",uip_len);
        printf("%s",(char *)uip_appdata);
        uip_send((char *)tmp_dat,strlen(tmp_dat));
    }
}


void UDP_5566_APP(void)
{
    /* 判断当前状态 */
    
    if (uip_newdata())
    {
        char *tmp_dat = "5566 receive the data!\r\n";
        /* 收到新数据 */
        printf("%d ",uip_len);
        printf("%s",(char *)uip_appdata);
        uip_send((char *)tmp_dat,strlen(tmp_dat));
    }

	else if(udp_server_sta&(1<<5))//有数据需要发送
	{
		udp_server_sta&=~(1<<5);//清除标记
		uip_send((char *)udp_server_databuf,strlen((const char*)udp_server_databuf));
	} 

	else if (uip_poll())
    {
//        char *tmp_dat = "the 5566 auto send!\r\n";
//        uip_send((char *)tmp_dat,strlen(tmp_dat));
    }	
}
























