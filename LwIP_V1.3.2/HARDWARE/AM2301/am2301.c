#include "am2301.h"
#include "bsp_ticktimer.h"


uint16_t ShiDu,WenDu;
uint16_t ShiDu_Count=0;//��ǰ�ɼ����ڼ�������
uint16_t WenDu_Count=0;
uint16_t ShiDu_Array[10]={0};//�����˲���ʷ����
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
/************��ʼ�����ɼ�����**************/
/******************************************/
void AM2301_Read_Init(void)
{
	unsigned char tr_shiZ,tr_shiX,tr_wenZ,tr_wenX,check,temp;
	DHT_PORT_OUT;
	Clr_DHT;		  //����ʹDHT11�͵�ƽ����ʱ����18ms
	bsp_DelayMS(30);
	Set_DHT;		  //������DHT11�ߵ�ƽ20~40us,���ȴ��ӻ���Ӧ
	bsp_DelayUS(40);
	DHT_PORT_IN;
	if((GPIOC->IDR&0x00000002)==0)  //�ӻ�������Ӧ�ź�
	{
		while((GPIOC->IDR&0x00000002)==0);
		while((GPIOC->IDR&0x00000002)!=0);	//��ʼ�ɼ�����
		tr_shiZ=AM2301_Read_Data();//�ɼ�ʪ����������
		tr_shiX=AM2301_Read_Data();//�ɼ�ʪ��С������
		tr_wenZ=AM2301_Read_Data();//�ɼ��¶���������
		tr_wenX=AM2301_Read_Data();//�ɼ��¶�С������
		check=AM2301_Read_Data();	//�ɼ�У��λ				
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
/*************��ʪ�ȶ�ȡ����***************/
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
