#ifndef __AM2301_H
#define __AM2301_H 			   
#include "stm32f10x.h"


#define DHT_PORT_IN 	(GPIOC->CRL &= 0xFFFFFF0F,GPIOC->CRL |= 0x00000080,GPIOC->BSRR = 0x00000002)
#define DHT_PORT_OUT 	(GPIOC->CRL &= 0xFFFFFF0F,GPIOC->CRL |= 0x00000030)
#define Set_DHT 		(GPIOC->BSRR = 0x00000002)
#define Clr_DHT 		(GPIOC->BRR = 0x00000002)

extern uint16_t ShiDu,WenDu;
extern uint16_t ShiDu_Count;//当前采集到第几个数据
extern uint16_t WenDu_Count;
extern uint16_t ShiDu_Array[10];//递推滤波历史数据
extern uint16_t WenDu_Array[10];

void GPIO_AM2301_Config(void);
void AM2301_Read_Init(void);
unsigned char AM2301_Read_Data(void);


#endif


