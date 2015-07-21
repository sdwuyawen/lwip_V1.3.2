#include "stm32f10x.h"
#include "platform_config.h"
#include "usart.h"

////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos ʹ��	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/8/18
//�汾��V1.5
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_REC_LEN,���ڶ��崮�����������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��
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

#if EN_USART1_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	  

static void usart_init_gpio(void);
static void usart_init_NVIC(void);

void USART_Send_N_Bytes(USART_TypeDef* USARTx, u8* Data,u8 nNum)
{
	u8 Send_Count=0;
	for(Send_Count=0;Send_Count<nNum;Send_Count++)
	{
		while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);/*�ȴ��������*/
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
  /*ʹ�ܴ���1ʹ�õ�GPIOʱ��*/
  RCC_APB2PeriphClockCmd(USART1_GPIO_CLK, ENABLE);
  /*ʹ�ܴ���2ʹ�õ�GPIOʱ��*/
  RCC_APB2PeriphClockCmd(USART2_GPIO_CLK, ENABLE); 
  /*ʹ�ܴ���1��2ʹ�õ�AFIOʱ��*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  /*����1 RX�ܽ�����*/
  /* Configure USART1 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = USART1_RxPin;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(USART1_GPIO, &GPIO_InitStructure);

  /*����2 RX�ܽ�����*/
  /* Configure USART2 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = USART2_RxPin;
  GPIO_Init(USART2_GPIO, &GPIO_InitStructure);  

  /*����1 TX�ܽ�����*/ 
  /* Configure USART1 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = USART1_TxPin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(USART1_GPIO, &GPIO_InitStructure);

  /*����2 TX�ܽ�����*/ 
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
  /*ʹ�ܴ���1ʱ��*/
  RCC_APB2PeriphClockCmd(USART1_CLK, ENABLE);  
  /*ʹ�ܴ���2ʱ��*/
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
  USART_InitStructure.USART_BaudRate = 115200;               /*���ò�����Ϊ9600*/
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*��������λΪ8*/
  USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*����ֹͣλΪ1λ*/
  USART_InitStructure.USART_Parity = USART_Parity_No;        /*����żУ��*/
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;/*��Ӳ������*/
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*���ͺͽ���*/
 
  /*���ô���1 */
  USART_Init(USART1, &USART_InitStructure);
  /*���ô���2*/
  USART_Init(USART2, &USART_InitStructure);
 
//   /*ʹ�ܴ���1�ķ��ͺͽ����ж�*/
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
   USART_ITConfig(USART1, USART_IT_TC, ENABLE);

  USART_ClearITPendingBit(USART1,USART_IT_RXNE);
  USART_ClearITPendingBit(USART1,USART_IT_TC);
//   /*ʹ�ܴ���2�ķ��ͺͽ����ж�*/
//   USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//   USART_ITConfig(USART2, USART_IT_TC, ENABLE);

//   USART_ClearITPendingBit(USART2,USART_IT_RXNE);
//   USART_ClearITPendingBit(USART2,USART_IT_TC);

  /* ʹ�ܴ���1 */
  USART_Cmd(USART1, ENABLE);
  /* ʹ�ܴ���2 */
  USART_Cmd(USART2, ENABLE);
}


void USART1_IRQHandler(void)
{
	u8 Res;
	#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
		OSIntEnter();    
	#endif
	if(USART_GetITStatus(USART1,USART_IT_TC) != RESET)//�����ж�
	{
		USART_ClearITPendingBit(USART1,USART_IT_TC);  
	}
	if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)//�����ж�
	{
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
		
		Res =USART_ReceiveData(USART1);//(USART1->DR);	//��ȡ���յ�������
		if((USART_RX_STA&0x8000)==0)//����δ���
		{
			if(USART_RX_STA&0x4000)//���յ���0x0d
			{
				if(Res!=0x0a)USART_RX_STA=0;//���մ���,���¿�ʼ
				else USART_RX_STA|=0x8000;	//��������� 
			}
			else //��û�յ�0X0D
			{	
				if(Res==0x0d)USART_RX_STA|=0x4000;
				else
				{
					USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
					USART_RX_STA++;
					if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;//�������ݴ���,���¿�ʼ����	  
				}		 
			}
		}  
	}
	#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
		OSIntExit();  											 
	#endif
}


#endif
