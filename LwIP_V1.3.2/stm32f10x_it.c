/**
  ******************************************************************************
  * @file    USART/Interrupt/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    04/16/2010
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
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
#include "stm32f10x_it.h"
#include "platform_config.h"
#include "sys_m3.h"	//IO口位带操作宏定义

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}



/******************************************************************************/
/*            STM32F10x Peripherals Interrupt Handlers                        */
/******************************************************************************/

/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */
/*
  USART_ClearITPendingBit(USART1,USART_IT_RXNE);
  USART_ClearITPendingBit(USART1,USART_IT_TC);
  或者
  USART_ClearFlag(USART1,USART_FLAG_RXNE);
  USART_ClearFlag(USART1,USART_FLAG_TC);
*/
/*
  USART_GetITStatus(USART1,USART_IT_RXNE)
  USART_GetITStatus(USART1,USART_IT_TC)
  或者
  USART_GetFlagStatus(USART1, USART_FLAG_RXNE)
  USART_GetFlagStatus(USART1, USART_FLAG_TC)
*/


/**
  * @brief  This function handles USART2 global interrupt request.
  * @param  None
  * @retval None
  */
#define USART_HEADER0 0x5A
#define USART_HEADER1 0xA5

void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2,USART_IT_TC) != RESET)//发送中断
	{
		USART_ClearITPendingBit(USART2,USART_IT_TC);  
	}
	if(USART_GetITStatus(USART2,USART_IT_RXNE) != RESET)//接收中断
	{
		USART_ClearITPendingBit(USART2,USART_IT_RXNE);	
	}
}


void TIM3_IRQHandler(void)   //TIM3中断，5ms
{
	u8 key_temp;
	static uint8_t s_count = 0;
	
	if (TIM_GetITStatus(TIM3,TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIMx的中断待处理位:TIM 中断源 
	
	}
}

void TIM4_IRQHandler(void)   //TIM4中断，1s
{
	if (TIM_GetITStatus(TIM4,TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update); 
		Tim4_irq_count++;		

	}
}

void DMA1_Channel4_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC4) != RESET)
	{
		DMA_ClearITPendingBit(DMA1_IT_TC4);
	    
	}

}

/**
  * @brief  This function handles SDIO global interrupt request.
  * @param  None
  * @retval None
  */
void SDIO_IRQHandler(void)
{
  /* Process All SDIO Interrupt Sources */

}

/*******************************************************************************
* Function Name  : EXTI9_5_IRQHandler
* Description    : This function handles External lines 9 to 5 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI9_5_IRQHandler(void) /* TouchScreen */
{
  if(EXTI_GetITStatus(EXTI_Line7) != RESET)
  {
//     //printf("\n\r tp");
//     ARMJISHU_TouchScreen_ADS7843();
//     
    /* Clear the EXTI Line 5 */  
    EXTI_ClearITPendingBit(EXTI_Line7);
  }
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
