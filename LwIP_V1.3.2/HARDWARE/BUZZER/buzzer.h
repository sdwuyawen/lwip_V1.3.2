#ifndef __BUZZER_H
#define __BUZZER_H 			   
#include "stm32f10x.h"


# define EN_BuzzerCtrl        1   /* 蜂鸣器控制使能,1有效 */
/*
-------------------------------------------------------------------------------
     当EN_BuzzerCtrl设为1时,程序将可以调用BuzzerSet()函数自由设定蜂鸣器的响声,
 从而可以用不同的声音指示不同的状态以方便调试,但这会增大系统开销.
-------------------------------------------------------------------------------
*/

                                               
/**************************数据定义**************************/

                                 

/*****声音设置*****/
# define BZR_TimeOut    36,4,2,0,0           /*处理超时*/
# define BZR_ReStart    120,55,5,0,0         /*异常复位*/
# define BZR_DebugErr   0,25,5,5,5           /*调试工具出错*/
# define BZR_LostStop   570,90,20,30,50       /*丢场停车*/
# define BZR_TimeStop   275,30,25,0,0        /*定时停车*/
# define BZR_StartLine  300,10,10,0,0        /*检测到起跑线*/
# define BZR_SDError   975,300,25,0,0        /*写卡完成*/
# define BZR_SDStop   480,10,50,50,10        /*写卡错误*/
# define BZR_EnTail   170,100,10,20,40       /*尾舵开启*/
# define BZR_EncodeErr   1800,200,10,200,40       /*编码器错误*/
/*
-------------------------------------------------------------------------------
     声音设置中的5个参数分别表示：
 (1)响声持续的总时间,=0表示不响(单位:场)
 (2)第一周期内,蜂鸣器开启的时间(单位:场)
 (3)第一周期内,蜂鸣器关闭的时间(单位:场)
 (4)第二周期内,蜂鸣器开启的时间(单位:场)
 (5)第二周期内,蜂鸣器关闭的时间(单位:场)
-------------------------------------------------------------------------------
*/


/*蜂鸣器管脚定义*/
#define BEEPER_PIN                GPIO_Pin_10		   /*蜂鸣器使用的GPIO管脚*/
#define GPIO_BEEPER               GPIOB			       /*神舟III号蜂鸣器使用的GPIO组*/
#define BUZZER_GPIO_CLK           RCC_APB2Periph_GPIOB /*蜂鸣器使用的GPIO时钟*/

#define ON_Buzzer()  (GPIO_ResetBits(GPIO_BEEPER, BEEPER_PIN))        /*开启蜂鸣器*/
#define OFF_Buzzer() (GPIO_SetBits(GPIO_BEEPER, BEEPER_PIN))     /*关闭蜂鸣器*/


/**************************常数定义**************************/
#define NUM_BuzzerQ   6    /*蜂鸣器控制缓冲队列的大小*/   
//#define EN_BuzzerCtrl        1   /* 蜂鸣器控制使能,1有效 */
/*
-------------------------------------------------------------------------------
     当EN_BuzzerCtrl设为1时,程序将可以调用BuzzerSet()函数自由设定蜂鸣器的响声,
 从而可以用不同的声音指示不同的状态以方便调试,但这会增大系统开销.
-------------------------------------------------------------------------------
*/


/**************************数据类型定义**************************/
typedef struct             /*蜂鸣器控制的数据类型*/
{
  unsigned int Tim_All;          /*响声持续的总时间(单位:场)*/
  unsigned char  Tim_On1;          /*第一周期内开启的时间(单位:场)*/
  unsigned char  Tim_Off1;         /*第一周期内关闭的时间(单位:场)*/
  unsigned char  Tim_On2;          /*第二周期内开启的时间(单位:场)*/
  unsigned char  Tim_Off2;         /*第二周期内关闭的时间(单位:场)*/
} BuzzerData_t;




/**************************接口函数声明**************************/
extern void BuzzerSet(u16 tim_all,u16 tim_on1,u16 tim_off1,u16 tim_on2,u16 tim_off2);
extern void BuzzerCtrl_Init(void);
extern void Process_BuzzerCtrl(void);


void GPIO_BUZZER_Config(void);




#endif
