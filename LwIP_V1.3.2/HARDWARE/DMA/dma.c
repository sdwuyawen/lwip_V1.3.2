#include "dma.h"

void DMA_Configuration(void)
{
  DMA_InitTypeDef DMA_InitStructure;
  /* DMA clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  /* USARTy TX DMA1 Channel (triggered by USARTy Tx event) Config */
  DMA_DeInit(DMA1_Channel4);//USART1_TXͨ��   
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);//�����ַ
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0; //�ڴ����ݵ�ַ  ��ԭ����usart_senddata
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;//�Ӵ洢����
  DMA_InitStructure.DMA_BufferSize = 10;//Ҫ���͵��ֽ�����
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//�����ַ������
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//�ڴ��ַ������
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel4, &DMA_InitStructure);

  /* USARTy RX DMA1 Channel (triggered by USARTy Rx event) Config */
  DMA_DeInit(DMA1_Channel5);//USART1_RXͨ�� 
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);//�����ַ
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;//�ڴ����ݵ�ַ��ԭ����usart_receivedata
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//�������
  DMA_InitStructure.DMA_BufferSize = 20;//Ҫ���յ��ֽ�����
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_Init(DMA1_Channel5, &DMA_InitStructure);

  DMA_ITConfig(DMA1_Channel4,DMA_IT_TC,ENABLE);
  DMA1_NVIC_Init();

  /* Enable USART1 DMA Rx and TX request */
  USART_DMACmd(USART1, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE);
  
//  /* USARTz TX DMA1 Channel (triggered by USARTz Tx event) Config */
//  DMA_DeInit(USARTz_Tx_DMA_Channel);  
//  DMA_InitStructure.DMA_PeripheralBaseAddr = USARTz_DR_Base;
//  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)TxBuffer2;
//  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
//  DMA_InitStructure.DMA_BufferSize = TxBufferSize2;  
//  DMA_Init(USARTz_Tx_DMA_Channel, &DMA_InitStructure);
//  
//  /* USARTz RX DMA1 Channel (triggered by USARTz Rx event) Config */
//  DMA_DeInit(USARTz_Rx_DMA_Channel);  
//  DMA_InitStructure.DMA_PeripheralBaseAddr = USARTz_DR_Base;
//  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)RxBuffer2;
//  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
//  DMA_InitStructure.DMA_BufferSize = TxBufferSize1;
//  DMA_Init(USARTz_Rx_DMA_Channel, &DMA_InitStructure);  
}
void DMA_USART1_Start(void)
{
//  DMA_InitTypeDef DMA_InitStructure;
//  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)usart_senddata; //�ڴ����ݵ�ַ
//  DMA_InitStructure.DMA_BufferSize = 10;//Ҫ���͵��ֽ�����
//  DMA_Init(DMA1_Channel4, &DMA_InitStructure);
  DMA_Configuration();
  /* Enable USART1 TX DMA1 Channel��������ʼDMA���� */
  DMA_Cmd(DMA1_Channel4, ENABLE);
  /* Enable USART1 RX DMA1 Channel��������ʼDMA���� */
  DMA_Cmd(DMA1_Channel5, ENABLE);
}
void DMA1_NVIC_Init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  	
}

