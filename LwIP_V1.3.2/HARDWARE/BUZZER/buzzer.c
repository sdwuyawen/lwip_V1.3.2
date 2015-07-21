#include "buzzer.h"

#if EN_BuzzerCtrl != 0

static unsigned int Tim_Buzzer;                  /*������ʱ�����*/
static BuzzerData_t BuzzerQ[NUM_BuzzerQ];  /*�������������*/
static unsigned char BuzzerQ_Head,BuzzerQ_Rear;    /*������������е�ͷβָ��*/
static unsigned char BuzzerQ_Stat;                 /*�������������״̬,=0��ʾ����*/

#endif

#if EN_BuzzerCtrl != 0
/************************************************************************************************ 
* BuzzerCtrl_Init
* ���������Ƴ�ʼ��
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
* ���ݷ����������е����ݽ��з���������
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
    { /*���Ӳ���*/
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
* �趨������������(!!!ע��:�ú������߱��ɳ�����,��ֹ���ж��������е���!!!)
* ��ڲ���  tim_all:������������ʱ��(��λ:��)
            tim_on1:��һ�����ڷ�����������ʱ��(��λ:��)
            tim_off1:��һ�����ڷ������رյ�ʱ��(��λ:��)
            tim_on2:�ڶ������ڷ�����������ʱ��(��λ:��)
            tim_off2:�ڶ������ڷ������رյ�ʱ��(��λ:��)
************************************************************************************************/
void BuzzerSet(u16 tim_all,u16 tim_on1,u16 tim_off1,u16 tim_on2,u16 tim_off2)
{
#if EN_BuzzerCtrl != 0
  
  BuzzerData_t *pbzr; 
  
  if(tim_all != 0 && BuzzerQ_Stat != 0)
  { /*����������Ϣ���*/
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
    
    /*�������ж�*/
    if(BuzzerQ_Rear == BuzzerQ_Head)    
    {
      BuzzerQ_Stat = 0;
    }
  }

#else

  (void)tim_all;      /*����������ľ���*/
  (void)tim_on1;
  (void)tim_off1;
  (void)tim_on2;
  (void)tim_off2;
      
#endif
}

void GPIO_BUZZER_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

  	/*ʹ�ܷ�����ʹ�õ�GPIOʱ��*/
	RCC_APB2PeriphClockCmd(BUZZER_GPIO_CLK, ENABLE);

  	/*��ʼ��������ʹ�õ�GPIO�ܽ�*/
  	GPIO_InitStructure.GPIO_Pin = BEEPER_PIN;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_Init(GPIO_BEEPER, &GPIO_InitStructure);

	GPIO_SetBits(GPIO_BEEPER, BEEPER_PIN);     /*�رշ�����*/
}
