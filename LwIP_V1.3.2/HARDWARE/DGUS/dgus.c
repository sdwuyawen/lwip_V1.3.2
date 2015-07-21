#include "dgus.h"
#include "stdio.h"//用于printf

DGUSINFO DGUS_Info;

void DGUS_Draw_Sin(u8 line_num,u16 line_data)
{ 
	usart_senddata[0]=0x5A;
	usart_senddata[1]=0xA5;
	usart_senddata[2]=0x04;
	usart_senddata[3]=0x84;
	usart_senddata[4]=line_num+1;
	usart_senddata[5]=(u8)(((u16)line_data)/256);
	usart_senddata[6]=(u8)(((u16)line_data)%256);	
	USART_Send_N_Bytes(USART2,usart_senddata,7);
}
void DGUS_RD_Page(void)
{
	usart_senddata[0]=0x5A;
	usart_senddata[1]=0xA5;
	usart_senddata[2]=0x03;
	usart_senddata[3]=0x81;
	usart_senddata[4]=0x03;
	usart_senddata[5]=0x02;	
	USART_Send_N_Bytes(USART2,usart_senddata,6);	
}
void DGUS_WR_Page(u16 Page_Num)
{
	usart_senddata[0]=0x5A;
	usart_senddata[1]=0xA5;
	usart_senddata[2]=0x04;
	usart_senddata[3]=0x80;
	usart_senddata[4]=0x03;
	usart_senddata[5]=Page_Num/256;
	usart_senddata[6]=Page_Num%256;	
	USART_Send_N_Bytes(USART2,usart_senddata,7);	
}
void DGUS_Clr_TP_Flag(void)
{
	usart_senddata[0]=0x5A;
	usart_senddata[1]=0xA5;
	usart_senddata[2]=0x03;
	usart_senddata[3]=0x80;
	usart_senddata[4]=0x05;	
	usart_senddata[5]=0x00;
	USART_Send_N_Bytes(USART2,usart_senddata,6);
}
void DGUS_RD_TP_Flag(void)
{
	usart_senddata[0]=0x5A;
	usart_senddata[1]=0xA5;
	usart_senddata[2]=0x03;
	usart_senddata[3]=0x81;
	usart_senddata[4]=0x05;	
	usart_senddata[5]=0x01;
	USART_Send_N_Bytes(USART2,usart_senddata,6);	
}
void DGUS_RD_Touch_Pos(void)
{
	usart_senddata[0]=0x5A;
	usart_senddata[1]=0xA5;
	usart_senddata[2]=0x03;
	usart_senddata[3]=0x81;
	usart_senddata[4]=0x07;	
	usart_senddata[5]=0x04;
	USART_Send_N_Bytes(USART2,usart_senddata,6);	
}
void DGUS_Get_RetVal(void)
{
	if(usart_receive_rdy==1)//接收到来自DGUS的有效数据包
	{
		usart_receive_rdy=0;		
		if(usart_receivedata[3]==0x81)//返回是寄存器区的数据
		{
			switch(usart_receivedata[4])
			{
				case DGUS_PIC_ID://返回的是页面ID,usart_receivedata[5]是长度2
					if(usart_receivedata[5]==0x02)//usart_receivedata[5]正确
					{
						DGUS_Info.Ret_Type=DGUS_PAGE_NUM;
						DGUS_Info.Page_Num=(u16)usart_receivedata[6];
						DGUS_Info.Page_Num<<=8;
						DGUS_Info.Page_Num|=usart_receivedata[7];						
						DGUS_Info.UnDeal=1;														
					}
					break;
				case DGUS_TP_FLAG://返回的是touch坐标已经更新
					if(usart_receivedata[5]==0x01)//usart_receivedata[5]正确
					{												
						if(usart_receivedata[6]==0x5A)//坐标有更新
						{							
							DGUS_Info.Ret_Type=DGUS_TP_UPDATE;
							DGUS_Info.UnDeal=1;		
						}														
					}
					break;
				case TP_POSITION://返回的是有效touch坐标值
					if(usart_receivedata[5]==0x04)//返回字节数正确
					{
						DGUS_Info.Ret_Type=DGUS_TOUCH_VALID;
						DGUS_Info.Touch_X=(u16)usart_receivedata[6];
						DGUS_Info.Touch_X<<=8;
						DGUS_Info.Touch_X|=usart_receivedata[7];
						DGUS_Info.Touch_Y=(u16)usart_receivedata[8];
						DGUS_Info.Touch_Y<<=8;
						DGUS_Info.Touch_Y|=usart_receivedata[9];
						DGUS_Info.Touch_Flag=1;//读取到有效touch
						DGUS_Info.UnDeal=1;	
					}
					break;
				case DGUS_TPC_Enable://返回的是是否使能触控
					break;
				default:
					break;
			}
		}
		if(usart_receivedata[3]==0x83)//返回是变量区的数据
		{
			DGUS_Info.Var_Address=(u16)usart_receivedata[4];
			DGUS_Info.Var_Address<<=8;
			DGUS_Info.Var_Address|=usart_receivedata[5];			
			switch(DGUS_Info.Var_Address)//读取变量地址
			{
				case DGUS_KEY_Page0://是页面0上的按键
					if(usart_receivedata[6]==0x01)//usart_receivedata[6]正确
					{
						DGUS_Info.Ret_Type=DGUS_PRESS_KEY;
						DGUS_Info.Key_Value=(u16)usart_receivedata[7];
						DGUS_Info.Key_Value<<=8;
						DGUS_Info.Key_Value|=usart_receivedata[8];
						DGUS_Info.UnDeal=1;														
					}
					break;
				default:
					break;
			}
		}
//		USART_Send_N_Bytes(USART1,usart_receivedata,usart_receive_longth+3);
	}	
}
void DGUS_Deal_RetVal(void)
{
	DGUS_Get_RetVal();//获得返回值的类型和值
	if(DGUS_Info.UnDeal)//有有效的返回值
	{
		DGUS_Info.UnDeal=0;
		if(DGUS_Info.Ret_Type==DGUS_PAGE_NUM)//返回的是页面ID
		{
//			USART_Send_N_Bytes(USART1,(u8*)(&DGUS_Info.Page_Num+1),1);
//			USART_Send_N_Bytes(USART1,(u8*)(&DGUS_Info.Page_Num),1);
			printf("获得页面ID=%d\n", DGUS_Info.Page_Num);			
		}
		if(DGUS_Info.Ret_Type==DGUS_PRESS_KEY)//返回的是按键
		{
			if(DGUS_Info.Var_Address == DGUS_KEY_Page0)//是页面0上的按键
			{
				printf("页面0，键值=%x\n", DGUS_Info.Key_Value);
				if(DGUS_Info.Key_Value==DGUS_KEY4_Page0)
				{
					DGUS_Info.Page_Num = DGUS_ID_Page1;
					DGUS_WR_Page(DGUS_Info.Page_Num);	
				}	
			}		
		}
		if(DGUS_Info.Ret_Type==DGUS_TP_UPDATE)//返回的是坐标已经更新
		{
			DGUS_Clr_TP_Flag();//清TP_FLAG
			DGUS_RD_Touch_Pos();//读按下的坐标		
		}
		if(DGUS_Info.Ret_Type==DGUS_TOUCH_VALID)//读取到有效坐标值
		{
			DGUS_Info.Touch_Flag = 0;
			printf("Touch X=%3d  Touch Y=%3d\n", DGUS_Info.Touch_X,DGUS_Info.Touch_Y);						
		}	
	}		
}
