#ifndef __KEY_H
#define __KEY_H 			   
#include "stm32f10x.h"

/*神州III号按键相关定义*/
#define RCC_KEY1                                    RCC_APB2Periph_GPIOD
#define GPIO_KEY1_PORT                              GPIOD    
#define GPIO_KEY1                                   GPIO_Pin_3

#define RCC_KEY2                                    RCC_APB2Periph_GPIOA
#define GPIO_KEY2_PORT                              GPIOA  
#define GPIO_KEY2                                   GPIO_Pin_8

#define RCC_KEY3                                    RCC_APB2Periph_GPIOC
#define GPIO_KEY3_PORT                              GPIOC    
#define GPIO_KEY3                                   GPIO_Pin_13 

#define RCC_KEY4                                    RCC_APB2Periph_GPIOA
#define GPIO_KEY4_PORT                              GPIOA    
#define GPIO_KEY4                                   GPIO_Pin_0 

#define GPIO_KEY_ANTI_TAMP                          GPIO_KEY3
#define GPIO_KEY_WEAK_UP                            GPIO_KEY4


/* Values magic to the Board keys */  
#define  KEY1_IN	GPIO_ReadInputDataBit(GPIO_KEY1_PORT, GPIO_KEY1)  
#define  KEY2_IN	GPIO_ReadInputDataBit(GPIO_KEY2_PORT, GPIO_KEY2)
#define  KEY3_IN	GPIO_ReadInputDataBit(GPIO_KEY3_PORT, GPIO_KEY3)
#define  KEY4_IN	GPIO_ReadInputDataBit(GPIO_KEY4_PORT, GPIO_KEY4)


/*短按返回的按键值*/
#define NOKEY 0x00
#define KEY0  0x01
#define KEY1  0x02
#define KEY2  0x04
#define KEY3  0x08
/*长按返回的按键值*/
#define KEY0L  0x81
#define KEY1L  0x82
#define KEY2L  0x84
#define KEY3L  0x88
/*返回组合长按按键值*/
#define KEYALLL 0x8F

#define KEY_CYCLETIME 5        /* 按键采样周期 毫秒 */ 
#define KEY_PRESS_TIME 1000     /* 长按时间,毫秒  */ 
#define KEY_SHORTPRESS_TIME 40 /* 短按时间,毫秒  */

extern u8 Key_Buff;

void GPIO_KEY_Config(void);
u8 encode(void);
u8 keydowith(void); 

#endif
