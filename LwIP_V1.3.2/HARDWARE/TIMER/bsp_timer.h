#ifndef __TIMER_H
#define __TIMER_H 			   
#include "stm32f10x.h"

extern uint16_t Tim4_start_time;
extern uint16_t Tim4_end_time;
extern uint16_t Tim4_cost_time;

extern uint16_t Tim4_irq_count;

void Timer3_Init(void);
void Timer4_Init(void);

#endif
