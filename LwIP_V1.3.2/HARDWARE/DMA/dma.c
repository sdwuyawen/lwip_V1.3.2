#include "dma.h"

void DMA_Configuration(void)
{
  DMA_InitTypeDef DMA_InitStructure;
  /* DMA clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  /* USARTy TX DMA1 Channel (triggered by USARTy Tx event) Config */
  DMA_DeInit(DMA1_Channel4);//USART1_TX通道   
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);//外设地址
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0; //内存数据地址  ，原来是usart_senddata
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;//从存储器读
  DMA_InitStructure.DMA_BufferSize = 10;//要传送的字节数量
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设地址不增加
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//内存地址自增加
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel4, &DMA_InitStructure);

  /* USARTy RX DMA1 Channel (triggered by USARTy Rx event) Config */
  DMA_DeInit(DMA1_Channel5);//USART1_RX通道 
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);//外设地址
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;//内存数据地址，原来是usart_receivedata
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//从外设读
  DMA_InitStructure.DMA_BufferSize = 20;//要接收的字节数量
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
//  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)usart_senddata; //内存数据地址
//  DMA_InitStructure.DMA_BufferSize = 10;//要传送的字节数量
//  DMA_Init(DMA1_Channel4, &DMA_InitStructure);
  DMA_Configuration();
  /* Enable USART1 TX DMA1 Channel，立即开始DMA发送 */
  DMA_Cmd(DMA1_Channel4, ENABLE);
  /* Enable USART1 RX DMA1 Channel，立即开始DMA接收 */
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

