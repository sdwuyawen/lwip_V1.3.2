#ifndef __DGUS_H
#define __DGUS_H 			   
#include "stm32f10x.h"

//������ɫ
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
#define BROWN 			 0XBC40 //��ɫ
#define BRRED 			 0XFC07 //�غ�ɫ
#define GRAY  			 0X8430 //��ɫ
//GUI��ɫ
#define DARKBLUE      	 0X01CF	//����ɫ
#define LIGHTBLUE      	 0X7D7C	//ǳ��ɫ  
#define GRAYBLUE       	 0X5458 //����ɫ
//������ɫΪPANEL����ɫ 
#define LIGHTGREEN     	 0X841F //ǳ��ɫ
#define LGRAY 			 0XC618 //ǳ��ɫ(PANNEL),���屳��ɫ
#define LGRAYBLUE        0XA651 //ǳ����ɫ(�м����ɫ)
#define LBBLUE           0X2B12 //ǳ����ɫ(ѡ����Ŀ�ķ�ɫ)
//ҳ���л�ʱʹ�õ�ҳ���
#define DGUS_ID_Page0 	 0x0000
#define DGUS_ID_Page1 	 0x0001
#define DGUS_ID_Page2 	 0x0002
#define DGUS_ID_Page3 	 0x0003
#define DGUS_ID_Page4 	 0x0004
#define DGUS_ID_Page5 	 0x0005
#define DGUS_ID_Page6 	 0x0006
//������DGUS�Ĵ�����
#define DGUS_PIC_ID 	 0x03	 //��ǰҳ��ID�Ĵ���
#define DGUS_TP_FLAG     0x05 //touch�����Ƿ���� 0x5A��ʾ�и��£�����ֵ�޸���
#define TP_POSITION      0x07 //��Ч����ֵ�Ĵ���
#define DGUS_TPC_Enable  0x0B //�Ƿ����ô��� 00������ FF����
//�����Ƿ���ֵ������
#define DGUS_RET_NONE    0x00;
#define DGUS_PAGE_NUM    0x01//���ص���ҳ���
#define DGUS_PRESS_KEY   0x02//���ص��ǰ�����ַ�Ͱ���ֵ
#define DGUS_TP_UPDATE   0x03//���ص���touch�Ƿ����
#define DGUS_TOUCH_VALID 0x04//���ص���touch��Ч����ֵ

//���¶���ÿ��ҳ�水����ַ
#define DGUS_KEY_Page0   0x0001
//���¶���ҳ�水��ֵ
#define DGUS_KEY0_Page0  0x0001
#define DGUS_KEY1_Page0  0x0002
#define DGUS_KEY2_Page0  0x0003
#define DGUS_KEY3_Page0  0x0004
#define DGUS_KEY4_Page0  0x0005


typedef struct 
{
	u8 UnDeal;//��δ����ķ���ֵ
	u8 Ret_Type;//����ֵ������
	u8 Touch_Flag;//Ϊ1ʱ����δ�������Ч����ֵ
	u16 Touch_X;//���µ�����ֵX
	u16 Touch_Y;//���µ�����ֵY
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
/*��������������ֱ�Ӷ�ȡtouch������ֵ*/
void DGUS_Clr_TP_Flag(void);
void DGUS_RD_TP_Flag(void);//�ú�����ȡtouch����ֵ
void DGUS_RD_Touch_Pos(void);

#endif

