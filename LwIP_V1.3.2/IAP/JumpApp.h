#ifndef __JUMPIAP_H
#define __JUMPIAP_H 			   
#include "stm32f10x.h"

#define ApplicationAddress    0X08005000    //APPµÿ÷∑
typedef  void (*pFunction)(void);


void GoToApp(void);

#endif
