#ifndef __DGUS_H
#define __DGUS_H 			   
#include "stm32f10x.h"

//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色
//GUI颜色
#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色  
#define GRAYBLUE       	 0X5458 //灰蓝色
//以上三色为PANEL的颜色 
#define LIGHTGREEN     	 0X841F //浅绿色
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)
//页面切换时使用的页面号
#define DGUS_ID_Page0 	 0x0000
#define DGUS_ID_Page1 	 0x0001
#define DGUS_ID_Page2 	 0x0002
#define DGUS_ID_Page3 	 0x0003
#define DGUS_ID_Page4 	 0x0004
#define DGUS_ID_Page5 	 0x0005
#define DGUS_ID_Page6 	 0x0006
//以下是DGUS寄存器号
#define DGUS_PIC_ID 	 0x03	 //当前页面ID寄存器
#define DGUS_TP_FLAG     0x05 //touch坐标是否更新 0x5A表示有更新，其他值无更新
#define TP_POSITION      0x07 //有效坐标值寄存器
#define DGUS_TPC_Enable  0x0B //是否启用触控 00不启用 FF启用
//以下是返回值的类型
#define DGUS_RET_NONE    0x00;
#define DGUS_PAGE_NUM    0x01//返回的是页面号
#define DGUS_PRESS_KEY   0x02//返回的是按键地址和按键值
#define DGUS_TP_UPDATE   0x03//返回的是touch是否更新
#define DGUS_TOUCH_VALID 0x04//返回的是touch有效坐标值

//以下定义每个页面按键地址
#define DGUS_KEY_Page0   0x0001
//以下定义页面按键值
#define DGUS_KEY0_Page0  0x0001
#define DGUS_KEY1_Page0  0x0002
#define DGUS_KEY2_Page0  0x0003
#define DGUS_KEY3_Page0  0x0004
#define DGUS_KEY4_Page0  0x0005


typedef struct 
{
	u8 UnDeal;//有未处理的返回值
	u8 Ret_Type;//返回值的类型
	u8 Touch_Flag;//为1时，有未处理的有效坐标值
	u16 Touch_X;//按下的坐标值X
	u16 Touch_Y;//按下的坐标值Y
	u16 Page_Num;
	u16 Var_Address;
	u16 Key_Value;
}
DGUSINFO;

extern DGUSINFO DGUS_Info;


void DGUS_Draw_Sin(u8 line_num,u16 line_data);
void DGUS_RD_Page(void);
void DGUS_WR_Page(u16 Page_Num);
void DGUS_Get_RetVal(void);
void DGUS_Deal_RetVal(void);
/*以下三个函数是直接读取touch的坐标值*/
void DGUS_Clr_TP_Flag(void);
void DGUS_RD_TP_Flag(void);//该函数读取touch坐标值
void DGUS_RD_Touch_Pos(void);

#endif

