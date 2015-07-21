#include "stm32f10x.h"
#include "platform_config.h"
#include "led.h"

void LED_config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /* Enable GPIOB, GPIOC and AFIO clock */
  RCC_APB2PeriphClockCmd(RCC_GPIO_LED , ENABLE);  //RCC_APB2Periph_AFIO
  
  /* LEDs pins configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_LED_ALL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIO_LED, &GPIO_InitStructure);

  GPIO_SetBits(GPIO_LED,DS1_PIN|DS2_PIN|DS3_PIN|DS4_PIN);/*关闭所有的LED指示灯*/
}

void Led_Turn_on_all(void)
{
	/* Turn On All LEDs */
    GPIO_ResetBits(GPIO_LED, GPIO_LED_ALL);
}

void Led_Turn_on_1(void)
{
	/* Turn On DS1 */
    GPIO_ResetBits(GPIO_LED, DS1_PIN);
}

void Led_Turn_on_2(void)
{
	/* Turn On DS2 */
    GPIO_ResetBits(GPIO_LED, DS2_PIN );
}

void Led_Turn_on_3(void)
{
	/* Turn On DS3 */
    GPIO_ResetBits(GPIO_LED, DS3_PIN);
}


void Led_Turn_on_4(void)
{
	/* Turn On DS4 */
    GPIO_ResetBits(GPIO_LED, DS4_PIN);
}

void Led_Turn_off_1(void)
{
	/* Turn Off DS1 */
    GPIO_SetBits(GPIO_LED, DS1_PIN);
}

void Led_Turn_off_2(void)
{
	/* Turn Off DS2 */
    GPIO_SetBits(GPIO_LED, DS2_PIN );
}

void Led_Turn_off_3(void)
{
	/* Turn Off DS3 */
    GPIO_SetBits(GPIO_LED, DS3_PIN);
}


void Led_Turn_off_4(void)
{
	/* Turn Off DS4 */
    GPIO_SetBits(GPIO_LED, DS4_PIN);
}

void Led_Turn_off_all(void)
{
	/* Turn Off All LEDs */
    GPIO_SetBits(GPIO_LED, GPIO_LED_ALL);
}

void Led_Turn_on(u8 led)
{
    Led_Turn_off_all();

	/* Turn Off Select LED */
    switch(led)
    {
        case 1:
          Led_Turn_on_1();
          break;

        case 2:
          Led_Turn_on_2();
          break;

        case 3:
          Led_Turn_on_3();
          break;

        case 4:
          Led_Turn_on_4();
          break;
          
        default:
          Led_Turn_on_all();
          break;
    }
}

void Led_Turn_on_Hex(u8 led)
{
    Led_Turn_off_all();

	/* Turn On Select LED */
    if((led >> 3) & 0x01)
    {
        Led_Turn_on_4();
    }
	else
	{
		Led_Turn_off_4();	
	}
    
    if((led >> 2) & 0x01)
    {
        Led_Turn_on_3();
    }
	else
	{
		Led_Turn_off_3();	
	}

    if((led >> 1) & 0x01)
    {
        Led_Turn_on_2();
    }
	else
	{
		Led_Turn_off_2();	
	}
    
    if((led >> 0) & 0x01)
    {
        Led_Turn_on_1();
    }
	else
	{
		Led_Turn_off_1();
	}
}
void LED_Toggle_1(void)
{
	if(GPIO_ReadInputDataBit(GPIO_LED, DS1_PIN)==SET)
	{
		/* Turn On DS1 */
	    GPIO_ResetBits(GPIO_LED, DS1_PIN);
	}
	else
	{
		GPIO_SetBits(GPIO_LED, DS1_PIN);
	}
}
void LED_Toggle_2(void)
{
	if(GPIO_ReadInputDataBit(GPIO_LED, DS2_PIN)==SET)
	{
		/* Turn On DS2 */
	    GPIO_ResetBits(GPIO_LED, DS2_PIN);
	}
	else
	{
		GPIO_SetBits(GPIO_LED, DS2_PIN);
	}
}



































