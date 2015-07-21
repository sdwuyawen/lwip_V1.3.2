#include "key.h"


u8 Key_Buff = NOKEY;


//全局变量声明

void GPIO_KEY_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Configure KEY1 Button */
  RCC_APB2PeriphClockCmd(RCC_KEY1, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = GPIO_KEY1;
  GPIO_Init(GPIO_KEY1_PORT, &GPIO_InitStructure);

  /* Configure KEY2 Button */
  RCC_APB2PeriphClockCmd(RCC_KEY2, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = GPIO_KEY2;
  GPIO_Init(GPIO_KEY2_PORT, &GPIO_InitStructure);

  /* Configure KEY3 Button */
  RCC_APB2PeriphClockCmd(RCC_KEY3, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = GPIO_KEY3;
  GPIO_Init(GPIO_KEY3_PORT, &GPIO_InitStructure);  

  /* Configure KEY4 Button */
  RCC_APB2PeriphClockCmd(RCC_KEY4, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = GPIO_KEY4;
  GPIO_Init(GPIO_KEY4_PORT, &GPIO_InitStructure);
}



u8 encode(void) 
{  
   u8 Key_Temp = NOKEY; 
    
   if((KEY1_IN) == 0)
	Key_Temp |= KEY0;
   if((KEY2_IN) == 0)
	Key_Temp |= KEY1;
   if((KEY3_IN) == 0)
	Key_Temp |= KEY2;
   if((KEY4_IN) == 0)
	Key_Temp |= KEY3;
   return(Key_Temp);		   	 
} 
//判断按键是否有效 定时器调用 
u8 keydowith(void) 
{ 
   u8 keyreturn=NOKEY;
   static u8 B_keymark = 0;            /* 有按键标志  */ 
   static u8 keypress = NOKEY;             /* 当前按键    */
   static u8 keypress_old = NOKEY;             /* 当前按键    */  
   static u16 keycontinue = 0;          /* 长按键计数  */ 
   static u8 keyreact = 0; 			/* 按键已响应标志 */ 

   keypress=encode(); 
   if (!B_keymark)                /* 第一次按键标志  */ 
   { 
      if(keypress!=NOKEY)          /* 第一次按键      */ 
      { 
		keyreact=0;//已响应标志清0
		B_keymark=1; 
		keycontinue=0; 
		keyreturn=NOKEY;
      } 
   } 
   else   
   { 
      if(keypress!=NOKEY)          /* 按键未释放      */ 
      { 
		keypress_old=keypress;
		if(keycontinue>(KEY_PRESS_TIME/KEY_CYCLETIME)) /*长按时间*/ 
		{ 
			if(keyreact==0)
			{
				keyreact=1;//已响应标志
				keycontinue=0; //重新计时 
				keyreturn=(keypress|0x80);//函数返回值等于长按键值
			}
		} 
		else keycontinue++; 
      } 
      else 	//按键已释放
      {  
		keypress=keypress_old;
		keypress_old=NOKEY;
		B_keymark=0;		  
		if(keycontinue>=(KEY_SHORTPRESS_TIME/KEY_CYCLETIME))
		{
			if(keyreact==0)
			{	
				keyreact=1;//已响应标志
				keyreturn=keypress;
				keypress=NOKEY;		 
			}
		}
		keycontinue=0;	
      } 
   }  
  return(keyreturn); 
} 


