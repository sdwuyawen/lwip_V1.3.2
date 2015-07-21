#ifndef __DMA_H
#define __DMA_H 			   
#include "stm32f10x.h"

void DMA_Configuration(void);
void DMA_USART1_Start(void);
void DMA1_NVIC_Init(void);

#endif
