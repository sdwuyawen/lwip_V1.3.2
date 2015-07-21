#include "am2301.h"
#include "bsp_ticktimer.h"


uint16_t ShiDu,WenDu;
uint16_t ShiDu_Count=0;//当前采集到第几个数据
uint16_t WenDu_Count=0;
uint16_t ShiDu_Array[10]={0};//递推滤波历史数据
uint16_t WenDu_Array[10]={0};


void GPIO_AM2301_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

}


/******************************************/
/************初始化及采集程序**************/
/******************************************/
void AM2301_Read_Init(void)
{
	unsigned char tr_shiZ,tr_shiX,tr_wenZ,tr_wenX,check,temp;
	DHT_PORT_OUT;
	Clr_DHT;		  //主机使DHT11低电平并延时至少18ms
	bsp_DelayMS(30);
	Set_DHT;		  //主机置DHT11高电平20~40us,并等待从机相应
	bsp_DelayUS(40);
	DHT_PORT_IN;
	if((GPIOC->IDR&0x00000002)==0)  //从机发出相应信号
	{
		while((GPIOC->IDR&0x00000002)==0);
		while((GPIOC->IDR&0x00000002)!=0);	//开始采集数据
		tr_shiZ=AM2301_Read_Data();//采集湿度整数部分
		tr_shiX=AM2301_Read_Data();//采集湿度小数部分
		tr_wenZ=AM2301_Read_Data();//采集温度整数部分
		tr_wenX=AM2301_Read_Data();//采集温度小数部分
		check=AM2301_Read_Data();	//采集校验位				
		while((GPIOC->IDR&0x00000002)==0);	
		Set_DHT;
		DHT_PORT_OUT;
		
		temp=tr_shiZ+tr_shiX+tr_wenZ+tr_wenX;
		if(check==temp)
		{
			ShiDu  = ((u16)tr_shiZ)<<8;
			ShiDu |= tr_shiX;
			WenDu =  ((u16)tr_wenZ)<<8;
			WenDu |= tr_wenX;
		}	
	}			   	
}
/******************************************/
/*************温湿度读取函数***************/
/******************************************/
unsigned char AM2301_Read_Data(void)
{
	unsigned char i,num,temp;
	num=0;
	for(i=0;i<8;i++)
	{
		while((GPIOC->IDR&0x00000002)==0);
		bsp_DelayUS(35);
		if((GPIOC->IDR&0x00000002)!=0) 
		{
			temp=1;
			while((GPIOC->IDR&0x00000002)!=0);				
		}
		else
			temp=0;
		num<<=1;
		num|=temp;
	} 
	return(num);
}
