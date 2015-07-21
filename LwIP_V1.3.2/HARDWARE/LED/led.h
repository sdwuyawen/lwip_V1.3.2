#ifndef __LED_H
#define __LED_H 			   
#include "stm32f10x.h"

/*神州III号LED灯相关定义*/
#define RCC_GPIO_LED                    RCC_APB2Periph_GPIOF    /*LED使用的GPIO时钟*/
#define LEDn                            4                       /*神舟III号LED数量*/
#define GPIO_LED                        GPIOF                   /*神舟III号LED灯使用的GPIO组*/

#define DS1_PIN                         GPIO_Pin_6              /*DS1使用的GPIO管脚*/
#define DS2_PIN                         GPIO_Pin_7				/*DS2使用的GPIO管脚*/
#define DS3_PIN                         GPIO_Pin_8  			/*DS3使用的GPIO管脚*/
#define DS4_PIN                         GPIO_Pin_9				/*DS4使用的GPIO管脚*/

#define GPIO_LED_ALL                                 DS1_PIN |DS2_PIN |DS3_PIN |DS4_PIN 

#define LED1 PFout(6)
#define LED2 PFout(7)
#define LED3 PFout(8)
#define LED4 PFout(9)

void LED_config(void);
void Led_Turn_on_all(void);
void Led_Turn_on_1(void);
void Led_Turn_on_2(void);
void Led_Turn_on_3(void);
void Led_Turn_on_4(void);
void Led_Turn_off_1(void);
void Led_Turn_off_2(void);
void Led_Turn_off_3(void);
void Led_Turn_off_4(void);
void Led_Turn_off_all(void);
void Led_Turn_on(u8 led);
void Led_Turn_on_Hex(u8 led);
void LED_Toggle_1(void);
void LED_Toggle_2(void);


#endif





























