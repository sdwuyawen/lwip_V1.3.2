/*printf支持文件*/


#include <stdio.h>
#include <rt_misc.h>
#include "stm32f10x.h"
#pragma import(__use_no_semihosting_swi)

static int SendChar(int ch); // 声明外部函数，在main文件中定义

// static int GetKey(void);

struct __FILE 
{

	int handle; // Add whatever you need here

};

FILE __stdout;

FILE __stdin;

/*添加支持printf函数*/
static int SendChar (int ch) 
{
	while (!(USART1->SR & USART_FLAG_TXE)); // USART1 可换成你程序中通信的串口	
	USART1->DR = (ch & 0x1FF);	
	return (ch);
}

// static int GetKey (void) 
// {
// 	while (!(USART1->SR & USART_FLAG_RXNE));
// 	return ((int)(USART1->DR & 0x1FF));
// }

int fputc(int ch, FILE *f) 
{
	return (SendChar(ch));
}

// int fgetc(FILE *f) 
// {
// 	return (SendChar(GetKey()));
// }

void _ttywrch(int ch) 
{
	SendChar (ch);
}

int ferror(FILE *f) 
{ // Your implementation of ferror
	return EOF;
}

void _sys_exit(int return_code) 
{
	label: goto label; // endless loop
}
