#include "key.h"


u8 Key_Buff = NOKEY;


//ȫ�ֱ�������

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
//�жϰ����Ƿ���Ч ��ʱ������ 
u8 keydowith(void) 
{ 
   u8 keyreturn=NOKEY;
   static u8 B_keymark = 0;            /* �а�����־  */ 
   static u8 keypress = NOKEY;             /* ��ǰ����    */
   static u8 keypress_old = NOKEY;             /* ��ǰ����    */  
   static u16 keycontinue = 0;          /* ����������  */ 
   static u8 keyreact = 0; 			/* ��������Ӧ��־ */ 

   keypress=encode(); 
   if (!B_keymark)                /* ��һ�ΰ�����־  */ 
   { 
      if(keypress!=NOKEY)          /* ��һ�ΰ���      */ 
      { 
		keyreact=0;//����Ӧ��־��0
		B_keymark=1; 
		keycontinue=0; 
		keyreturn=NOKEY;
      } 
   } 
   else   
   { 
      if(keypress!=NOKEY)          /* ����δ�ͷ�      */ 
      { 
		keypress_old=keypress;
		if(keycontinue>(KEY_PRESS_TIME/KEY_CYCLETIME)) /*����ʱ��*/ 
		{ 
			if(keyreact==0)
			{
				keyreact=1;//����Ӧ��־
				keycontinue=0; //���¼�ʱ 
				keyreturn=(keypress|0x80);//��������ֵ���ڳ�����ֵ
			}
		} 
		else keycontinue++; 
      } 
      else 	//�������ͷ�
      {  
		keypress=keypress_old;
		keypress_old=NOKEY;
		B_keymark=0;		  
		if(keycontinue>=(KEY_SHORTPRESS_TIME/KEY_CYCLETIME))
		{
			if(keyreact==0)
			{	
				keyreact=1;//����Ӧ��־
				keyreturn=keypress;
				keypress=NOKEY;		 
			}
		}
		keycontinue=0;	
      } 
   }  
  return(keyreturn); 
} 


