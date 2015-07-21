#include "buzzer.h"

#if EN_BuzzerCtrl != 0

static unsigned int Tim_Buzzer;                  /*蜂鸣器时间变量*/
static BuzzerData_t BuzzerQ[NUM_BuzzerQ];  /*蜂鸣器缓冲队列*/
static unsigned char BuzzerQ_Head,BuzzerQ_Rear;    /*蜂鸣器缓冲队列的头尾指针*/
static unsigned char BuzzerQ_Stat;                 /*蜂鸣器缓冲队列状态,=0表示队满*/

#endif

#if EN_BuzzerCtrl != 0
/************************************************************************************************ 
* BuzzerCtrl_Init
* 蜂鸣器控制初始化
************************************************************************************************/
void BuzzerCtrl_Init(void)
{
  BuzzerQ_Head = 0;
  BuzzerQ_Rear = 0;
  BuzzerQ_Stat = 1;
  Tim_Buzzer = 0;
}
/************************************************************************************************ 
* Process_BuzzerCtrl
* 根据蜂鸣器队列中的数据进行蜂鸣器控制
************************************************************************************************/
void Process_BuzzerCtrl(void)
{
  BuzzerData_t *pbzr;
  unsigned char  on1,off1,on2,off2;
  unsigned int  tim;
  
  if(BuzzerQ_Rear != BuzzerQ_Head || BuzzerQ_Stat == 0)
  {
    pbzr = &BuzzerQ[BuzzerQ_Head];
    on1 = pbzr->Tim_On1;
    off1 = pbzr->Tim_Off1;
    on2 = pbzr->Tim_On2;
    off2 = pbzr->Tim_Off2;
    tim = Tim_Buzzer % (on1 + off1 + on2 + off2);
    
    if(tim < on1 + off1)
    {
      if(tim < on1) 
      {
        ON_Buzzer();
      }
      else         
      {
        OFF_Buzzer();
      }
    }
    else
    {
      if(tim < on1 + off1 + on2)
      {
        ON_Buzzer();
      }
      else
      {
        OFF_Buzzer();
      }
    }
    
    if((++Tim_Buzzer) >= pbzr->Tim_All)
    { /*出队操作*/
      OFF_Buzzer();
      if((++BuzzerQ_Head) == NUM_BuzzerQ)
      {
        BuzzerQ_Head = 0;
      }
      Tim_Buzzer = 0;
      BuzzerQ_Stat = 1;
    }
  }
}

#endif
/************************************************************************************************ 
* BuzzerSet
* 设定蜂鸣器的响声(!!!注意:该函数不具备可冲入性,禁止在中断里对其进行调用!!!)
* 入口参数  tim_all:响声持续的总时间(单位:场)
            tim_on1:第一周期内蜂鸣器开启的时间(单位:场)
            tim_off1:第一周期内蜂鸣器关闭的时间(单位:场)
            tim_on2:第二周期内蜂鸣器开启的时间(单位:场)
            tim_off2:第二周期内蜂鸣器关闭的时间(单位:场)
************************************************************************************************/
void BuzzerSet(u16 tim_all,u16 tim_on1,u16 tim_off1,u16 tim_on2,u16 tim_off2)
{
#if EN_BuzzerCtrl != 0
  
  BuzzerData_t *pbzr; 
  
  if(tim_all != 0 && BuzzerQ_Stat != 0)
  { /*将响声的信息入队*/
    pbzr = &BuzzerQ[BuzzerQ_Rear];
    pbzr->Tim_All = tim_all;
    pbzr->Tim_On1 = tim_on1;
    pbzr->Tim_Off1 = tim_off1;
    pbzr->Tim_On2 = tim_on2;
    pbzr->Tim_Off2 = tim_off2;
    
    if((++BuzzerQ_Rear) == NUM_BuzzerQ)
    {
      BuzzerQ_Rear = 0; 
    }
    
    /*队满的判定*/
    if(BuzzerQ_Rear == BuzzerQ_Head)    
    {
      BuzzerQ_Stat = 0;
    }
  }

#else

  (void)tim_all;      /*避免编译器的警告*/
  (void)tim_on1;
  (void)tim_off1;
  (void)tim_on2;
  (void)tim_off2;
      
#endif
}

void GPIO_BUZZER_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

  	/*使能蜂鸣器使用的GPIO时钟*/
	RCC_APB2PeriphClockCmd(BUZZER_GPIO_CLK, ENABLE);

  	/*初始化蜂鸣器使用的GPIO管脚*/
  	GPIO_InitStructure.GPIO_Pin = BEEPER_PIN;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_Init(GPIO_BEEPER, &GPIO_InitStructure);

	GPIO_SetBits(GPIO_BEEPER, BEEPER_PIN);     /*关闭蜂鸣器*/
}
