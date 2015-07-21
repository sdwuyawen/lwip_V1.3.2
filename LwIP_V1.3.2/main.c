/**
  ******************************************************************************
  * @file    USART/Interrupt/main.c 
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    04/16/2010
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

#include "bsp.h"

#include "platform_config.h"
#include "sys_m3.h"	//IO��λ�������궨��
//#include "my_malloc.h"	//�Զ�����ڴ�ط���
#include "ff.h"
#include "mmc_sd.h"
#include "exfuns.h"
#include "usmart.h"
#include "gliderfilter.h"
#include "ili9320.h"
// #include "spi.h"
#include "spi_flash.h"
#include "fontupd.h"
#include "text.h"
#include "led.h"
#include "enc28j60.h"

#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/dhcp.h"
//#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#include "netif/ethernetif.h"	//mymac����
#include "arch/sys_arch.h"	

#include "./LWIP-APP/app_test/lwipdemo.h"



/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/


/* Private macro -------------------------------------------------------------*/
#define countof(a)   (sizeof(a) / sizeof(*(a)))

/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/


uint8_t Disp_Buf[1000];

uint8_t loop_count=0;

//Ҫд�뵽W25Q64���ַ�������
const u8 TEXT_Buffer[]={"WarShipSTM32 SPI TEST"};
#define SIZE sizeof(TEXT_Buffer)
u8 datatemp[SIZE];

uint32_t ENC_TXERIF_Cnt = 0;	//ENC���ʹ������
uint32_t ENC_RXERIF_Cnt = 0;	//ENC���մ������
uint32_t ENC_RX_Pkt_Total_Cnt = 0;	//�������ݰ�����
uint32_t ENC_RX_Pkt_Valid_Cnt = 0;	//������Ч���ݰ�����
uint32_t ENC_RX_Pkt_inValid_Cnt = 0;	//������Ч���ݰ�����
uint32_t ENC_RX_Pkt_inValid_Bytes = 0;	//�ܽ�����Ч�ֽ���
uint32_t ENC_BreakDown_Cnt = 0;		//����N���ղ������ݣ���λENC28J60
uint32_t ENC_Reset_Times = 0;		//оƬ��λ����
uint32_t ENC_TX_Reset_Cnt = 0;		//�����߼���λ����
uint32_t ENC_RX_Reset_Cnt = 0;		//�����߼���λ����
uint16_t ENC_RX_Disp_Cnt = 0;		//����ָʾ��

void NVIC_Configuration(void);
void Key_All_Disposal(void);
void Key_KEY0_Disposal(void);
void Key_KEY1_Disposal(void);
void Key_KEY2_Disposal(void);
void Key_KEY3_Disposal(void);
void Key_KEY0L_Disposal(void);
void Key_KEY1L_Disposal(void);
void Key_KEY2L_Disposal(void);
void Key_KEY3L_Disposal(void);
void Key_KEYALLL_Disposal(void);





/* Private functions ---------------------------------------------------------*/


/**
  * @brief   Main program
  * @param  None
  * @retval None
  */


int main(void)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file
     */      
	uint8_t ucKeyCode;		/* �������� */
	uint32_t ulPktCnt_Last = 0;	/* ��һ�����ݰ����������ڼ������հ�/s */
	uint32_t ulPktsPerSec = 0;	/* ÿ����հ��� */
	
	bsp_Init();
	
//	GPIO_KEY_Config();
	
// 	if(KEY1_IN==0||KEY2_IN==0||KEY3_IN==0||KEY4_IN==0)//��IAPģʽ
// 	{
// 		usart_init();
// 		SerialPutString("======================================================================\r\n");
// 		SerialPutString("=              (C) COPYRIGHT 2010 STMicroelectronics                 =\r\n");
// 		SerialPutString("=     In-Application Programming Application  (Version 3.2.0)        =\r\n");
// 		SerialPutString("=                                   By MCD Application Team          =\r\n");
// 		SerialPutString("======================================================================\r\n");
// 		Main_Menu ();
// 	}
// 	else//��ת��APP
// 	{
// 		SerialPutString("��תAPP.....\r\n");
// 		GoToApp();
// 	}
	
	NVIC_Configuration();
//	Delay_Init(72);	     //��ʱ��ʼ�� 
	LED_config();
    Timer3_Init();//ʹ�ܶ�ʱ��3,5ms 
	Timer4_Init();//ʹ�ܶ�ʱ��4,1000ms
	usart_init();
//	GPIO_BUZZER_Config();
	
	//��ʼ��������
	LCD_Init();

	//��ʼ��AM2301
//	GPIO_AM2301_Config();
	//��ʼ��������
#if EN_BuzzerCtrl != 0
	BuzzerCtrl_Init();
#endif	
//	my_mem_init(SRAMIN);	//��ʼ���ڲ��ڴ��	
	
	bsp_StartAutoTimer(0, 1000);	/* ����1��1000ms���Զ���װ�Ķ�ʱ�� */
	
//	usmart_dev.init(72);	
	
//	exfuns_init();					//Ϊfatfs��ر��������ڴ�  
//  f_mount(0,fs[0]); 		 		//����SD�� 
//  f_mount(1,fs[1]); 				//����FLASH.


/*
//	SPI_Flash_Init();  		//SPI FLASH ��ʼ�� 
	printf("SPI_FLASH_TYPE=%x\r\n",SPI_FLASH_TYPE);
	
	while(font_init()||KEY1_IN==0) //����ֿ�
	{
   
		LCD_Clear(WHITE);		   	//����
 		POINT_COLOR=RED;			//��������Ϊ��ɫ	   	   	  
		LCD_ShowString(60,50,200,16,16,"Warship STM32");
		while(SD_Initialize())		//���SD��
		{
			LCD_ShowString(60,70,200,16,16,"SD Card Failed!");
			bsp_DelayMS(200);
			LCD_Fill(60,70,200+60,70+16,WHITE);
			bsp_DelayMS(200);		    
		}								 						    
		LCD_ShowString(60,70,200,16,16,"SD Card OK");
		LCD_ShowString(60,90,200,16,16,"Font Updating...");
		key=update_font(20,110,16,0);//��SD������
		while(key)//����ʧ��		
		{			 		  
			LCD_ShowString(60,110,200,16,16,"Font Update Failed!");
			bsp_DelayMS(200);
			LCD_Fill(20,110,200+20,110+16,WHITE);
			bsp_DelayMS(200);		       
		} 		  
		LCD_ShowString(60,110,200,16,16,"Font Update Success!");
		bsp_DelayMS(1500);	
		LCD_Clear(WHITE);//����	       
	}  
	
	POINT_COLOR=RED;      
	Show_Str(60,50,200,16,"ս�� STM32������",16,0);				    	 
	Show_Str(60,70,200,16,"GBK�ֿ���Գ���",16,0);				    	 
	Show_Str(60,90,200,16,"FB ʵ����",16,0);				    	 
	Show_Str(60,110,200,16,"2012��9��18��",16,0);
	Show_Str(60,130,200,16,"��KEY0,�����ֿ�",16,0);
 	POINT_COLOR=BLUE;  
	Show_Str(60,150,200,16,"������ֽ�:",16,0);				    	 
	Show_Str(60,170,200,16,"������ֽ�:",16,0);				    	 
	Show_Str(60,190,200,16,"��Ӧ����(16*16)Ϊ:",16,0);			 
	Show_Str(60,212,200,12,"��Ӧ����(12*12)Ϊ:",12,0);			 
	Show_Str(60,230,200,16,"���ּ�����:",16,0);
	LCD_Fill(60,130,200+60,130+16,WHITE);	
*/	


//	POINT_COLOR=RED;		//����Ϊ��ɫ	   
//	LCD_ShowString(60,10,200,16,16,"WarShip STM32");	
//	LCD_ShowString(60,30,200,16,16,"ENC28J60 TEST");	

// 	sprintf((char*)Disp_Buf,"TXERR %d\r\n",ENC_TXERIF_Cnt);	
// 	LCD_ShowString(60,10,200,16,16,Disp_Buf);	
// 	
// 	sprintf((char*)Disp_Buf,"RXERR %d\r\n",ENC_RXERIF_Cnt);	
// 	LCD_ShowString(60,30,200,16,16,Disp_Buf);	
	
	
//	LCD_ShowString(60,50,200,16,16,"ATOM@ALIENTEK");

//  	while(tapdev_init())	//��ʼ��ENC28J60����
// 	{								   
// 		LCD_ShowString(60,70,200,16,16,"ENC28J60 Init Error!");	 
// 		bsp_DelayMS(200);
//  		LCD_Fill(60,70,240,86,WHITE);//���֮ǰ��ʾ
// 	};		

// 	LCD_ShowString(60,70,200,16,16,"KEY0:Server Send Msg");	 
// 	LCD_ShowString(60,90,200,16,16,"KEY2:Client Send Msg");	  
// 	LCD_ShowString(60,110,200,16,16,"IP:202,194,201,68");	   						  	 
// 	LCD_ShowString(60,130,200,16,16,"MASK:255.255.255.0");	   						  	 
// 	LCD_ShowString(60,150,200,16,16,"GATEWAY:202,194,201,254");	   						  	 
	
//  LCD_ShowString(30,200,200,16,16,"TCP RX:");	   						  	 
// 	LCD_ShowString(30,220,200,16,16,"TCP TX:");	
// 	   						  	   						  	 
// 	LCD_ShowString(30,270,200,16,16,"TCP RX:");	   						  	 
// 	LCD_ShowString(30,290,200,16,16,"TCP TX:");	   						  	 
//	POINT_COLOR=BLUE;	   

	LwIP_APP_Init();	//��ʼ��LwIP

	printf(("Ready to poll\r\n"));
	
	while (1)
	{	
		LWIP_Polling();  		//LwIP��ѯ
		
		/* �����˲��ͼ���ɺ�̨systick�жϷ������ʵ�֣�����ֻ��Ҫ����bsp_GetKey��ȡ��ֵ���ɡ� */
		ucKeyCode = bsp_GetKey();	/* ��ȡ��ֵ, �޼�����ʱ���� KEY_NONE = 0 */
		if (ucKeyCode != KEY_NONE)
		{
			switch (ucKeyCode)
			{
				case KEY_DOWN_K1:			/* K1������ */
					
					ENC28J60_Reg_Print();
				
					break;

				case KEY_DOWN_K2:			/* K2������ */			
					ENC28J60_Reg_Print();
				
					break;
							
				case KEY_DOWN_K3:			/* K3������ */
					ENC28J60_RX_Reset();	/* �����ո�λ */
				
				case KEY_DOWN_K4:			/* K4������ */
					ENC28J60_Full_Reset((uint8_t *)mymac);	/* ENC28J60��ȫ��λ */
				
				default:
					/* �����ļ�ֵ������ */
					break;
			}
		}
		
		bsp_Idle();		/* ���������bsp.c�ļ����û������޸��������ʵ��CPU���ߺ�ι�� */
		
		if (bsp_CheckTimer(0))	/* �ж϶�ʱ����ʱʱ�� */
		{
			/* ÿ��1000ms ����һ�� */
			LED_Toggle_1();	/* ��תLED4��״̬ */
			
			ulPktsPerSec = ENC_RX_Pkt_Total_Cnt - ulPktCnt_Last;	/* ������հ�/s */
			ulPktCnt_Last = ENC_RX_Pkt_Total_Cnt ;					/* ������һ�ν������ݰ��� */
			
			ucEIE = ENC28J60_Read(EIE);
			ucEIR = ENC28J60_Read(EIR);
			ucESTAT = ENC28J60_Read(ESTAT);
			ucECON1 = ENC28J60_Read(ECON1);
			ucECON2 = ENC28J60_Read(ECON2);
			ucEPKTCNT = ENC28J60_Read(EPKTCNT);
			
			ucERXWRPTL = ENC28J60_Read(ERXWRPTL);
			ucERXWRPTH = ENC28J60_Read(ERXWRPTH);
			ucERXRDPTL = ENC28J60_Read(ERXRDPTL);
			ucERXRDPTH = ENC28J60_Read(ERXRDPTH);
			
			usERXWRPT = ucERXWRPTL + (ucERXWRPTH << 8);
			usERXRDPT = ucERXRDPTL + (ucERXRDPTH << 8);
			
			if(usERXWRPT > usERXRDPT)
			{
				ulFreeSpace = (RXSTOP_INIT - RXSTART_INIT) - (usERXWRPT - usERXRDPT);
			}
			else if(usERXWRPT == usERXRDPT)
			{
				ulFreeSpace = (RXSTOP_INIT - RXSTART_INIT);
			}
			else
			{
				ulFreeSpace = usERXRDPT - usERXWRPT - 1;
			}
				
		
			//���������ݰ���
			sprintf((char*)Disp_Buf,"PktTotalCnt %d ",ENC_RX_Pkt_Total_Cnt);	
			LCD_ShowString(0,16*0,200,16,16,Disp_Buf);	
			
			//������Ч���ݰ���
			sprintf((char*)Disp_Buf,"PktValidCnt %d ",ENC_RX_Pkt_Valid_Cnt);	
			LCD_ShowString(0,16*1,200,16,16,Disp_Buf);	
			
			//������Ч���ݰ���
			sprintf((char*)Disp_Buf,"BadCnt %d ",ENC_RX_Pkt_Total_Cnt - ENC_RX_Pkt_Valid_Cnt);	
			LCD_ShowString(160,16*1,200,16,16,Disp_Buf);	
			
			//���ݰ������ٶȣ���/s
			sprintf((char*)Disp_Buf,"PktsPerSec %d ",ulPktsPerSec);	
			LCD_ShowString(0,16*2,200,16,16,Disp_Buf);
			
			//�������ݰ����ֽ���
			sprintf((char*)Disp_Buf,"BadByte %d ",ENC_RX_Pkt_inValid_Bytes);	
			LCD_ShowString(110,16*2,200,16,16,Disp_Buf);
			
			
			//�������ٴ�û���յ����ݰ�
			sprintf((char*)Disp_Buf,"BreakDown_Cnt %d ",ENC_BreakDown_Cnt);	
			LCD_ShowString(0,16*3,200,16,16,Disp_Buf);
			
			//ENC��λ����
			sprintf((char*)Disp_Buf,"Reset_Times %d ",ENC_Reset_Times);	
			LCD_ShowString(0,16*4,200,16,16,Disp_Buf);
			
			//��ʾENC28J60�ؼ��Ĵ���
			sprintf((char*)Disp_Buf,"EIE=0x%0.2x,EIR=0x%0.2x,ESTAT=0x%0.2x, ECON1=0x%0.2x,ECON2=0x%0.2x,EPKTCNT=0x%0.2x ", ucEIE, ucEIR, ucESTAT, ucECON1, ucECON2, ucEPKTCNT);	
			LCD_ShowString(0,260,240,48,16,Disp_Buf);
			
			//��ʾ�Ĵ���
			sprintf((char*)Disp_Buf,"ERXWRPTH=0x%0.2x ERXWRPTL=0x%0.2x ",ucERXWRPTH,ucERXWRPTL);	
			LCD_ShowString(0,16*5,240,16,16,Disp_Buf);
			
			//��ʾ�Ĵ���
			sprintf((char*)Disp_Buf,"ERXRDPTH=0x%0.2x ERXRDPTL=0x%0.2x ",ucERXRDPTH,ucERXRDPTL);	
			LCD_ShowString(0,16*6,240,16,16,Disp_Buf);
			
			//��ʾ������ʣ���ֽ���
			sprintf((char*)Disp_Buf,"FreeSpace=0x%0.4x=%4d ",ulFreeSpace,ulFreeSpace);	
			LCD_ShowString(0,16*7,240,16,16,Disp_Buf);
			
			//��ʾ�����߼���λ����
			sprintf((char*)Disp_Buf,"TXResetCnt=%d ",ENC_TX_Reset_Cnt);	
			LCD_ShowString(0,16*8,240,16,16,Disp_Buf);
			
			//��ʾ�����߼���λ����
			sprintf((char*)Disp_Buf,"RXResetCnt=%d ",ENC_RX_Reset_Cnt);	
			LCD_ShowString(0,16*9,240,16,16,Disp_Buf);
			
			
		}
		
		bsp_DelayMS(10);
	}
}

/**
  * @brief  Configures the nested vectored interrupt controller.
  * @param  None
  * @retval None
  */
void NVIC_Configuration(void)
{
//	NVIC_SetVectorTable(NVIC_VectTab_FLASH,0x0);
	/* Configure the NVIC Preemption Priority Bits */  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif




/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
