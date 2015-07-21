#include "bsp_timer.h"

uint16_t Tim4_start_time=0;
uint16_t Tim4_end_time=0;
uint16_t Tim4_cost_time=0;

uint16_t Tim4_irq_count=0;

static void Timer3_NVIC_Init(void);
static void Timer4_NVIC_Init(void);


/*APB1 = HCLK/2 = 36M*/
/*Timer2 - Timer7 时钟频率是APB1的2倍，即72M*/
void Timer3_Init(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
	Timer3_NVIC_Init();

	TIM_TimeBaseStructure.TIM_Period = 50 - 1; 	    /* 50 * 100us = 5ms*/
	TIM_TimeBaseStructure.TIM_Prescaler =7200 - 1;	/* 10K Hz*/ 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 
 
	TIM_ITConfig(TIM3, TIM_IT_Update,ENABLE);
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  

	TIM_Cmd(TIM3, ENABLE);  //使能TIMx外设
							 
}
static void Timer3_NVIC_Init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  

}

/*APB1 = HCLK/2 = 36M*/
/*Timer2 - Timer7 时钟频率是APB1的2倍，即72M*/
void Timer4_Init(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //时钟使能
  	Timer4_NVIC_Init();

	TIM_TimeBaseStructure.TIM_Period = 1000-1; 	    /* 1000 * 100us = 100ms*/
	TIM_TimeBaseStructure.TIM_Prescaler =7200 - 1;	/* 1M Hz*/ 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); 
 
 	TIM_ITConfig(TIM4, TIM_IT_Update,ENABLE);
 	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);  

	TIM_Cmd(TIM4, ENABLE);  //使能TIMx外设				 
}
static void Timer4_NVIC_Init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  

}
