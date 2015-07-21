#include "stm32f10x.h"
#include "platform_config.h"
#include "usart.h"

////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//串口1初始化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
 

#define USART1_GPIO              GPIOA
#define USART1_CLK               RCC_APB2Periph_USART1
#define USART1_GPIO_CLK          RCC_APB2Periph_GPIOA
#define USART1_RxPin             GPIO_Pin_10
#define USART1_TxPin             GPIO_Pin_9
#define USART1_IRQn              USART1_IRQn
#define USART1_IRQHandler        USART1_IRQHandler

#define USART2_GPIO              GPIOA
#define USART2_CLK               RCC_APB1Periph_USART2
#define USART2_GPIO_CLK          RCC_APB2Periph_GPIOA
#define USART2_RxPin             GPIO_Pin_3
#define USART2_TxPin             GPIO_Pin_2
#define USART2_IRQn              USART2_IRQn
#define USART2_IRQHandler        USART2_IRQHandler

#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  

static void usart_init_gpio(void);
static void usart_init_NVIC(void);

void USART_Send_N_Bytes(USART_TypeDef* USARTx, u8* Data,u8 nNum)
{
	u8 Send_Count=0;
	for(Send_Count=0;Send_Count<nNum;Send_Count++)
	{
		while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);/*等待发送完成*/
		USART_SendData(USARTx,*(Data+Send_Count));
	}	
}

void USART1_Send_Byte(u8 byte)
{
	USART_SendData(USART1,byte);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
}

void usart_init_gpio(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /*使能串口1使用的GPIO时钟*/
  RCC_APB2PeriphClockCmd(USART1_GPIO_CLK, ENABLE);
  /*使能串口2使用的GPIO时钟*/
  RCC_APB2PeriphClockCmd(USART2_GPIO_CLK, ENABLE); 
  /*使能串口1和2使用的AFIO时钟*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  /*串口1 RX管脚配置*/
  /* Configure USART1 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = USART1_RxPin;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(USART1_GPIO, &GPIO_InitStructure);

  /*串口2 RX管脚配置*/
  /* Configure USART2 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = USART2_RxPin;
  GPIO_Init(USART2_GPIO, &GPIO_InitStructure);  

  /*串口1 TX管脚配置*/ 
  /* Configure USART1 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = USART1_TxPin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(USART1_GPIO, &GPIO_InitStructure);

  /*串口2 TX管脚配置*/ 
  /* Configure USART2 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = USART2_TxPin;
  GPIO_Init(USART2_GPIO, &GPIO_InitStructure);  
}
void usart_init_NVIC(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Enable the USART1 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 6;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  /* Enable the USART2 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 7;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
void usart_init(void)
{
/* USART1 and USART2 configuration ------------------------------------------------------*/
  USART_InitTypeDef USART_InitStructure; 
  usart_init_NVIC();
  /*使能串口1时钟*/
  RCC_APB2PeriphClockCmd(USART1_CLK, ENABLE);  
  /*使能串口2时钟*/
  RCC_APB1PeriphClockCmd(USART2_CLK, ENABLE);
  usart_init_gpio();
  /* USART1 and USART2 configured as follow:
        - BaudRate = 9600 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 115200;               /*设置波特率为9600*/
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*设置数据位为8*/
  USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*设置停止位为1位*/
  USART_InitStructure.USART_Parity = USART_Parity_No;        /*无奇偶校验*/
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;/*无硬件流控*/
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*发送和接收*/
 
  /*配置串口1 */
  USART_Init(USART1, &USART_InitStructure);
  /*配置串口2*/
  USART_Init(USART2, &USART_InitStructure);
 
//   /*使能串口1的发送和接收中断*/
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
   USART_ITConfig(USART1, USART_IT_TC, ENABLE);

  USART_ClearITPendingBit(USART1,USART_IT_RXNE);
  USART_ClearITPendingBit(USART1,USART_IT_TC);
//   /*使能串口2的发送和接收中断*/
//   USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//   USART_ITConfig(USART2, USART_IT_TC, ENABLE);

//   USART_ClearITPendingBit(USART2,USART_IT_RXNE);
//   USART_ClearITPendingBit(USART2,USART_IT_TC);

  /* 使能串口1 */
  USART_Cmd(USART1, ENABLE);
  /* 使能串口2 */
  USART_Cmd(USART2, ENABLE);
}


void USART1_IRQHandler(void)
{
	u8 Res;
	#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
		OSIntEnter();    
	#endif
	if(USART_GetITStatus(USART1,USART_IT_TC) != RESET)//发送中断
	{
		USART_ClearITPendingBit(USART1,USART_IT_TC);  
	}
	if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)//接收中断
	{
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
		
		Res =USART_ReceiveData(USART1);//(USART1->DR);	//读取接收到的数据
		if((USART_RX_STA&0x8000)==0)//接收未完成
		{
			if(USART_RX_STA&0x4000)//接收到了0x0d
			{
				if(Res!=0x0a)USART_RX_STA=0;//接收错误,重新开始
				else USART_RX_STA|=0x8000;	//接收完成了 
			}
			else //还没收到0X0D
			{	
				if(Res==0x0d)USART_RX_STA|=0x4000;
				else
				{
					USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
					USART_RX_STA++;
					if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;//接收数据错误,重新开始接收	  
				}		 
			}
		}  
	}
	#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
		OSIntExit();  											 
	#endif
}


#endif
