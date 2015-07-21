#include "uip.h"	    
#include "udp_demo.h"
#include "string.h"
#include "stdio.h"
//////////////////////////////////////////////////////////////////////////////////	 
//ALIENTEKս��STM32������
//uIP TCP���� ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/28
//�汾��V1.0			   								  
//////////////////////////////////////////////////////////////////////////////////

u8 udp_server_databuf[200];   	//�������ݻ���	  
u8 udp_server_sta;				//�����״̬


void UDP_5555_APP(void);
void UDP_5566_APP(void);

//UDPӦ�ýӿں���(UIP_UDP_APPCALL)
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
    /* �жϵ�ǰ״̬ */
    if (uip_poll())
    {
        char *tmp_dat = "the 5555 auto send!\r\n";
        uip_send((char *)tmp_dat,strlen(tmp_dat));
    }
    if (uip_newdata())
    {
        char *tmp_dat = "5555 receive the data!\r\n";
        /* �յ������� */
        printf("%d ",uip_len);
        printf("%s",(char *)uip_appdata);
        uip_send((char *)tmp_dat,strlen(tmp_dat));
    }
}


void UDP_5566_APP(void)
{
    /* �жϵ�ǰ״̬ */
    
    if (uip_newdata())
    {
        char *tmp_dat = "5566 receive the data!\r\n";
        /* �յ������� */
        printf("%d ",uip_len);
        printf("%s",(char *)uip_appdata);
        uip_send((char *)tmp_dat,strlen(tmp_dat));
    }

	else if(udp_server_sta&(1<<5))//��������Ҫ����
	{
		udp_server_sta&=~(1<<5);//������
		uip_send((char *)udp_server_databuf,strlen((const char*)udp_server_databuf));
	} 

	else if (uip_poll())
    {
//        char *tmp_dat = "the 5566 auto send!\r\n";
//        uip_send((char *)tmp_dat,strlen(tmp_dat));
    }	
}
























