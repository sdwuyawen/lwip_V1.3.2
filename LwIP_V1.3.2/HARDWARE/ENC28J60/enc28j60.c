#include "spi.h"
//#include "delay.h"
#include <stdio.h>
#include "enc28j60.h"	
#include "bsp_ticktimer.h"
//#include "netif/ethernetif.h"

//#include "lwip/init.h"
//#include "lwip/ip.h"
//#include "lwip/dhcp.h"
//#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
//#include "lwip/dns.h"
//#include "netif/etharp.h"
#include "netif/ethernetif.h"
//#include "arch/sys_arch.h"


//////////////////////////////////////////////////////////////////////////////////	 
//ALIENTEKս��STM32������
//ENC28J60���� ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/28
//�汾��V1.0			   								  
//////////////////////////////////////////////////////////////////////////////////

#define ENC_printf(...)
//#define ENC_printf	printf

static u8 ENC28J60BANK;
static u32 NextPacketPtr;

uint8_t ucEIE;	/* ENC28J60�Ĵ���ֵ�����ڵ��Դ�ӡ */
uint8_t ucEIR;
uint8_t ucESTAT;
uint8_t ucECON1;
uint8_t ucECON2;
uint8_t ucEPKTCNT;

uint8_t ucERXWRPTL;
uint8_t ucERXWRPTH;
uint8_t ucERXRDPTL;
uint8_t ucERXRDPTH;

uint16_t usERXWRPT;
uint16_t usERXRDPT;
uint32_t ulFreeSpace;


//��λENC28J60
//����SPI��ʼ��/IO��ʼ����

static void ENC28J60_SPI1_Init(void)
{
/*
   	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_SPI1,  ENABLE );//SPI2ʱ��ʹ�� 	
   	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOG, ENABLE );//PORTB,D,Gʱ��ʹ�� 
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;				 // �˿�����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
 	GPIO_Init(GPIOD, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOD.2
 	GPIO_SetBits(GPIOD,GPIO_Pin_2);						 //PD.2����
			  
	//����PG7��PB12����,��Ϊ�˷�ֹNRF24L01��SPI FLASHӰ��.
	//��Ϊ���ǹ���һ��SPI��.  

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;				 // PB12 ����   ����
 	GPIO_Init(GPIOB, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOB.12
 	GPIO_SetBits(GPIOB,GPIO_Pin_12);						 //PB.12����

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8;//PG6/7/8 ����  ����			 
 	GPIO_Init(GPIOG, &GPIO_InitStructure);					 //�����趨������ʼ��//PG6/7/8
 	GPIO_SetBits(GPIOG,GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8);//PG6/7/8����	
			
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //PB13/14/15����������� 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOB

 	GPIO_SetBits(GPIOB,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);  //PB13/14/15����


	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//����SPI����ģʽ:����Ϊ��SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;		//����ͬ��ʱ�ӵĿ���״̬Ϊ�͵�ƽ
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;	//����ͬ��ʱ�ӵĵ�һ�������أ��������½������ݱ�����
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;		//���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRCֵ����Ķ���ʽ
	SPI_Init(SPI2, &SPI_InitStructure);  //����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���
 
	SPI_Cmd(SPI2, ENABLE); //ʹ��SPI����
	
	SPI2_ReadWriteByte(0xff);//��������		
*/
 	GPIO_InitTypeDef GPIO_InitStructure;
  	SPI_InitTypeDef  SPI_InitStructure;

	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE );//PORTAʱ��ʹ�� 
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_SPI1,  ENABLE );//SPI1ʱ��ʹ�� 
	
	/* ����Flash��ƬѡPC4 �� ENC28J60��ƬѡPA4 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;  // PC4 ���� 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOC,GPIO_Pin_4);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;  // PA4 ���죬����3��ENC28J60Ƭѡ 
 	GPIO_Init(GPIOA, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;  // PA2 ���죬���ENC28J60Ƭѡ 
 	GPIO_Init(GPIOA, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOA,GPIO_Pin_2);
 
	/* ����SPI1��SCK��MOSI */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOA,GPIO_Pin_5 | GPIO_Pin_7); 
	
	/* ����SPI1��MISO */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;	//MISOҪ���ó�����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
 	GPIO_SetBits(GPIOA,GPIO_Pin_6); 

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//����SPI����ģʽ:����Ϊ��SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;		//����ͬ��ʱ�ӵĿ���״̬Ϊ�͵�ƽ
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;	//����ͬ��ʱ�ӵĵ�һ�������أ��������½������ݱ�����
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;		//���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ16
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRCֵ����Ķ���ʽ
	SPI_Init(SPI1, &SPI_InitStructure);  //����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���
 
	SPI_Cmd(SPI1, ENABLE); //ʹ��SPI����
	
	SPI1_ReadWriteByte(0xff);//��������

}
void ENC28J60_Reset(void)
{
 	 
	ENC28J60_SPI1_Init();//SPI2��ʼ��
	SPI1_SetSpeed(SPI_BaudRatePrescaler_4);	//SPI2 SCKƵ��Ϊ72M/4=18Mhz
//  TIM6_Int_Init(1000,719);//100Khz����Ƶ�ʣ�������1000Ϊ10ms
//	ENC28J60_RST=0;			//��λENC28J60
	bsp_DelayMS(10);	 
//	ENC28J60_RST=1;			//��λ����				    
	bsp_DelayMS(10);	 
}
//��ȡENC28J60�Ĵ���(��������) 
//op��������
//addr:�Ĵ�����ַ/����
//����ֵ:����������
u8 ENC28J60_Read_Op(u8 op,u8 addr)
{
	u8 dat=0;	 
	ENC28J60_CS=0;	 
	dat=op|(addr&ADDR_MASK);
	SPI1_ReadWriteByte(dat);
	dat=SPI1_ReadWriteByte(0xFF);
	//����Ƕ�ȡMAC/MII�Ĵ���,��ڶ��ζ��������ݲ�����ȷ��,���ֲ�29ҳ
 	if(addr&0x80)dat=SPI1_ReadWriteByte(0xFF);
	ENC28J60_CS=1;
	return dat;
}
//��ȡENC28J60�Ĵ���(��������) 
//op��������
//addr:�Ĵ�����ַ
//data:����
void ENC28J60_Write_Op(u8 op,u8 addr,u8 data)
{
	u8 dat = 0;	    
	ENC28J60_CS=0;			   
	dat=op|(addr&ADDR_MASK);
	SPI1_ReadWriteByte(dat);	  
	SPI1_ReadWriteByte(data);
	ENC28J60_CS=1;
}
//��ȡENC28J60���ջ�������
//len:Ҫ��ȡ�����ݳ���
//data:������ݻ�����(ĩβ�Զ���ӽ�����)
void ENC28J60_Read_Buf(u32 len,u8* data)
{
	ENC28J60_CS=0;			 
	SPI1_ReadWriteByte(ENC28J60_READ_BUF_MEM);
	while(len)
	{
		len--;			  
		*data=(u8)SPI1_ReadWriteByte(0);
		data++;
	}
	*data='\0';
	ENC28J60_CS=1;
}
//��ENC28J60д���ͻ�������
//len:Ҫд������ݳ���
//data:���ݻ����� 
void ENC28J60_Write_Buf(u32 len,u8* data)
{
	ENC28J60_CS=0;			   
	SPI1_ReadWriteByte(ENC28J60_WRITE_BUF_MEM);		 
	while(len)
	{
		len--;
		SPI1_ReadWriteByte(*data);
		data++;
	}
	ENC28J60_CS=1;
}
//����ENC28J60�Ĵ���Bank
//ban:Ҫ���õ�bank
void ENC28J60_Set_Bank(u8 bank)
{								    
	if((bank&BANK_MASK)!=ENC28J60BANK)//�͵�ǰbank��һ�µ�ʱ��,������
	{				  
		ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR,ECON1,(ECON1_BSEL1|ECON1_BSEL0));
		ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,(bank&BANK_MASK)>>5);
		ENC28J60BANK=(bank&BANK_MASK);
	}
}
//��ȡENC28J60ָ���Ĵ��� 
//addr:�Ĵ�����ַ
//����ֵ:����������
u8 ENC28J60_Read(u8 addr)
{						  
	ENC28J60_Set_Bank(addr);//����BANK		 
	return ENC28J60_Read_Op(ENC28J60_READ_CTRL_REG,addr);
}
//��ENC28J60ָ���Ĵ���д����
//addr:�Ĵ�����ַ
//data:Ҫд�������		 
void ENC28J60_Write(u8 addr,u8 data)
{					  
	ENC28J60_Set_Bank(addr);		 
	ENC28J60_Write_Op(ENC28J60_WRITE_CTRL_REG,addr,data);
}
//��ENC28J60��PHY�Ĵ���д������
//addr:�Ĵ�����ַ
//data:Ҫд�������		 
void ENC28J60_PHY_Write(u8 addr,u32 data)
{
	u16 retry=0;
	ENC28J60_Write(MIREGADR,addr);	//����PHY�Ĵ�����ַ
	ENC28J60_Write(MIWRL,data);		//д������
	ENC28J60_Write(MIWRH,data>>8);		   
	while((ENC28J60_Read(MISTAT)&MISTAT_BUSY)&&retry<0XFFF)retry++;//�ȴ�д��PHY����		  
}
//��ʼ��ENC28J60
//macaddr:MAC��ַ
//����ֵ:0,��ʼ���ɹ�;
//       1,��ʼ��ʧ��;
u8 ENC28J60_Init(u8* macaddr)
{			
	uint8_t res_init;
	ENC28J60_Reset();
	res_init = ENC28J60_Full_Reset(macaddr);	//ִ��оƬ��ȫ��λ
	
	return (res_init);
}
//��ȡEREVID
u8 ENC28J60_Get_EREVID(void)
{
	//��EREVID ��Ҳ�洢�˰汾��Ϣ�� EREVID ��һ��ֻ����
	//�ƼĴ���������һ��5 λ��ʶ����������ʶ�����ض���Ƭ
	//�İ汾��
	return ENC28J60_Read(EREVID);
}


//#include "uip.h"

extern uint32_t ENC_TX_Reset_Cnt;		//�����߼���λ����
//ͨ��ENC28J60�������ݰ�������
//len:���ݰ���С
//packet:���ݰ�
void ENC28J60_Packet_Send(u32 len,u8* packet)
{
	//Ҫ���͵��ֽ������ܳ���MAMXFL�ж�����ֽ������������ַ��ʹ��󣬼��ֲ�12.1.3
	if(len > MAX_FRAMELEN - 4)
	{
		len = MAX_FRAMELEN - 4;
	}
	
	//��λ�����߼������⡣�μ�Rev. B4 Silicon Errata point 12.
	if((ENC28J60_Read(EIR)&EIR_TXERIF))
	{
		ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_TXRST);
		ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR,ECON1,ECON1_TXRST);
		ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR,EIR,EIR_TXERIF);
		ENC_TX_Reset_Cnt ++;	//�����߼���λ����+1
	}
	
	//���÷��ͻ�������ַдָ�����
	ENC28J60_Write(EWRPTL,TXSTART_INIT&0xFF);
	ENC28J60_Write(EWRPTH,TXSTART_INIT>>8);
	//дÿ�������ֽڣ�0x00��ʾʹ��macon3�����ã� 
	ENC28J60_Write_Op(ENC28J60_WRITE_BUF_MEM,0,0x00);
	//�������ݰ������ͻ�����
	ENC_printf("len:%d\r\n",len);	//���ӷ������ݳ���
 	ENC28J60_Write_Buf(len,packet);
	//����TXNDָ�룬�Զ�Ӧ���������ݰ���С	   
	ENC28J60_Write(ETXNDL,(TXSTART_INIT+len)&0xFF);
	ENC28J60_Write(ETXNDH,(TXSTART_INIT+len)>>8);
 	//�������ݵ�����
	ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_TXRTS);
	//��λ�����߼������⡣�μ�Rev. B4 Silicon Errata point 12.
	//errsheet��д���ǰ�˫��ģʽ�´��ڸ����⣬ȫ˫������û�и�����
// 	if((ENC28J60_Read(EIR)&EIR_TXERIF))
// 	{
// 		ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR,ECON1,ECON1_TXRTS);
// 	}

	
}


#define ENC28J60_BREAKDOWN_THRESHOLD	1000		//����1000�ν��ղ������ݰ�����ΪоƬ��������λоƬ
extern uint32_t ENC_RX_Pkt_Total_Cnt;	//�������ݰ�����
extern uint32_t ENC_RX_Pkt_Valid_Cnt;	//������Ч���ݰ�����
extern uint32_t ENC_RX_Pkt_inValid_Cnt;	//������Ч���ݰ�����
extern uint32_t ENC_RX_Pkt_inValid_Bytes;	//�ܽ�����Ч�ֽ���
extern uint32_t ENC_BreakDown_Cnt;		//����N���ղ������ݣ���λENC28J60
extern uint32_t ENC_Reset_Times;		//оƬ��λ����
extern uint32_t ENC_RX_Reset_Cnt;		//�����߼���λ����
extern uint16_t ENC_RX_Disp_Cnt;		//����ָʾ��
//�������ȡһ�����ݰ�����
//maxlen:���ݰ����������ճ���
//packet:���ݰ�������
//����ֵ:�յ������ݰ�����(�ֽ�)					
u32 ENC28J60_Packet_Receive(u32 maxlen,u8* packet)
{
	u32 rxstat;
	u32 len;   
	u32 nextpackptr_odd;	//��������һ����ַ������errsheet��14��
	
	if(ENC28J60_Read(EPKTCNT)==0)	
	{
		ENC_BreakDown_Cnt ++;	//оƬֹͣ��������
		if(ENC_BreakDown_Cnt >= ENC28J60_BREAKDOWN_THRESHOLD)
		{
			ENC_BreakDown_Cnt = 0;
			ENC_Reset_Times ++;	//оƬ��λ����++
			ENC28J60_Full_Reset((uint8_t *)mymac);	//��ȫ��λоƬ
		}
		return 0;  //û���յ����ݰ�	  
	}
	else	//���յ����ݰ�
	{
		ENC_BreakDown_Cnt = 0;
		Led_Turn_on_2();		// ��ָʾ��������
		ENC_RX_Disp_Cnt = 0;
	}
		
	ENC_RX_Pkt_Total_Cnt ++;	//�������ݰ�����++
	
	//���ý��ջ�������ָ��
	ENC28J60_Write(ERDPTL,(NextPacketPtr));
	ENC28J60_Write(ERDPTH,(NextPacketPtr)>>8);	   
	// ����һ������ָ��
	NextPacketPtr = ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0);
	NextPacketPtr |= ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0)<<8;
	//�����ĳ���
	len = ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0);
	len |= ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0)<<8;
 	len -= 4; //ȥ��CRC����
	//��ȡ����״̬
	rxstat = ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0);
	rxstat |= ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0)<<8;
	//���ƽ��ճ���	
	if (len > maxlen)
	{
		len = maxlen;
	}		
	//���CRC�ͷ��Ŵ���
	// ERXFCON.CRCENΪĬ������,һ�����ǲ���Ҫ���.
	//��Ϊ�������Ѿ�ʹ����CRCУ�飬�����������˵����ȡ��״̬���������ˣ����ܵ�ԭ����
	//��һ�ζ�ȡ����һ�����ݰ�ָ�������(������������һ�εĽ��ջ��汻�ƻ���)
	//����״̬�����Ĵ��󣬻���������һ�����ݰ�ָ��Ĵ�����ɲ��������һ�����ݰ�ָ�����Ȼ��
	//���д��ERXRDPT���Ǵ����ֵ������ERXWRPTֻ�ܵ�ERXRDPT-1��ERXRDPT�Ĵ�����ܻ���ɽ��տ��û�����
	//�����С�����յ��¿��ý��ջ���ǳ�С�����ܽ����κ����ݰ�������ȫ���ҵ�
	if((rxstat&0x80)==0)	//��������˵�������˷ǳ��ǳ����صĴ��󣬱��봦������ô����
	{
		ENC_RX_Pkt_inValid_Cnt ++;	//��Ч���ݰ���+1
		ENC_RX_Pkt_inValid_Bytes += len + 4;	//��Ч���ݰ��ܳ��� += ���γ���
		len=0;//��Ч
		
		//�����ո�λû��Ч��������ֱ�ӽ�����ȫ��λ
//		ENC28J60_RX_Reset();
		ENC28J60_Full_Reset((uint8_t *)mymac);	//��ȫ��λоƬ
		ENC_RX_Reset_Cnt ++;
	}
	else 	//���ݰ���Ч
	{
		ENC_RX_Pkt_Valid_Cnt ++;		//��Ч���ݰ���++
		ENC28J60_Read_Buf(len,packet);//�ӽ��ջ������и������ݰ�	 
		//RX��ָ���ƶ�����һ�����յ������ݰ��Ŀ�ʼλ�� 
		//���ͷ����ǸղŶ��������ڴ�
		//����errsheet��14��������һ���ĵ�ַת��Ϊ������ַ
		if(NextPacketPtr == RXSTART_INIT)
		{
			nextpackptr_odd = RXSTOP_INIT;
		}
		else
		{
			nextpackptr_odd = NextPacketPtr - 1;
		}
		
		ENC28J60_Write(ERXRDPTL,(nextpackptr_odd));
		ENC28J60_Write(ERXRDPTH,(nextpackptr_odd)>>8);
		//�ݼ����ݰ���������־�����Ѿ��õ�������� 
		ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON2,ECON2_PKTDEC);
	}
	
	return(len);
}

/**
* ��ӡ�ؼ��Ĵ�������Ϣ
**/

void ENC28J60_Reg_Print(void)
{
	ucEIE = ENC28J60_Read(EIE);
	ucEIR = ENC28J60_Read(EIR);
	ucESTAT = ENC28J60_Read(ESTAT);
	ucECON1 = ENC28J60_Read(ECON1);
	ucECON2 = ENC28J60_Read(ECON2);
	ucEPKTCNT = ENC28J60_Read(EPKTCNT);
	
	ENC_printf("\r\nEIE=0x%0.2x,EIR=0x%0.2x,ESTAT=0x%0.2x,ECON1=0x%0.2x,ECON2=0x%0.2x,EPKTCNT=0x%0.2x \r\n", ucEIE, ucEIR, ucESTAT, ucECON1, ucECON2, ucEPKTCNT);
}


//ִ�н����ո�λ
void ENC28J60_RX_Reset(void)
{
	//�����ո�λ
	ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_RXRST);
	ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR,ECON1,ECON1_RXRST);
	
	ucERXWRPTL = ENC28J60_Read(ERXWRPTL);
	ucERXWRPTH = ENC28J60_Read(ERXWRPTH);
	
	ENC_printf("ERXWRPTH=0x%0.2x ERXWRPTL=0x%0.2x ",ucERXWRPTH,ucERXWRPTL);	
	
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers,must write low byte first
	// set receive buffer start address	   ���ý��ջ�������ַ  8K�ֽ�����
	NextPacketPtr=RXSTART_INIT;
	// Rx start
	//���ջ�������һ��Ӳ�������ѭ��FIFO ���������ɡ�
	//�Ĵ�����ERXSTH:ERXSTL ��ERXNDH:ERXNDL ��
	//Ϊָ�룬���建���������������ڴ洢���е�λ�á�
	//ERXST��ERXNDָ����ֽھ�������FIFO�������ڡ�
	//������̫���ӿڽ��������ֽ�ʱ����Щ�ֽڱ�˳��д��
	//���ջ������� ���ǵ�д����ERXND ָ��Ĵ洢��Ԫ
	//��Ӳ�����Զ������յ���һ�ֽ�д����ERXST ָ��
	//�Ĵ洢��Ԫ�� ��˽���Ӳ��������д��FIFO ����ĵ�
	//Ԫ��
	//���ý�����ʼ�ֽ�
	ENC28J60_Write(ERXSTL,RXSTART_INIT&0xFF);		//��д��ERXSTʱ��������ͬ��ֵ�Զ����ERXWRPT
	ENC28J60_Write(ERXSTH,RXSTART_INIT>>8);	  
	//ERXWRPTH:ERXWRPTL �Ĵ�������Ӳ����FIFO ��
	//���ĸ�λ��д������յ����ֽڡ� ָ����ֻ���ģ��ڳ�
	//�����յ�һ�����ݰ���Ӳ�����Զ�����ָ�롣 ָ���
	//�����ж�FIFO ��ʣ��ռ�Ĵ�С  8K-1500�� 
	//���ý��ն�ָ���ֽ�
	ENC28J60_Write(ERXRDPTL,RXSTART_INIT&0xFF);
	ENC28J60_Write(ERXRDPTH,RXSTART_INIT>>8);
	//���ý��ս����ֽ�
	ENC28J60_Write(ERXNDL,RXSTOP_INIT&0xFF);
	ENC28J60_Write(ERXNDH,RXSTOP_INIT>>8);
	
	ucERXWRPTL = ENC28J60_Read(ERXWRPTL);
	ucERXWRPTH = ENC28J60_Read(ERXWRPTH);
	
	ENC_printf("ERXWRPTH=0x%0.2x ERXWRPTL=0x%0.2x ",ucERXWRPTH,ucERXWRPTL);	
	
	//�������
	ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_RXEN);		
}


//ִ��ȫ����λ
uint8_t ENC28J60_Full_Reset(u8* macaddr)
{
	u16 retry=0;
	
	ENC28J60_Write_Op(ENC28J60_SOFT_RESET,0,ENC28J60_SOFT_RESET);//�����λ
	while(!(ENC28J60_Read(ESTAT)&ESTAT_CLKRDY)&&retry<500)//�ȴ�ʱ���ȶ�
	{
		retry++;
		bsp_DelayMS(1);
	};
	if(retry>=500)return 1;//ENC28J60��ʼ��ʧ��
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers,must write low byte first
	// set receive buffer start address	   ���ý��ջ�������ַ  8K�ֽ�����
	NextPacketPtr=RXSTART_INIT;
	// Rx start
	//���ջ�������һ��Ӳ�������ѭ��FIFO ���������ɡ�
	//�Ĵ�����ERXSTH:ERXSTL ��ERXNDH:ERXNDL ��
	//Ϊָ�룬���建���������������ڴ洢���е�λ�á�
	//ERXST��ERXNDָ����ֽھ�������FIFO�������ڡ�
	//������̫���ӿڽ��������ֽ�ʱ����Щ�ֽڱ�˳��д��
	//���ջ������� ���ǵ�д����ERXND ָ��Ĵ洢��Ԫ
	//��Ӳ�����Զ������յ���һ�ֽ�д����ERXST ָ��
	//�Ĵ洢��Ԫ�� ��˽���Ӳ��������д��FIFO ����ĵ�
	//Ԫ��
	//���ý�����ʼ�ֽ�
	ENC28J60_Write(ERXSTL,RXSTART_INIT&0xFF);	
	ENC28J60_Write(ERXSTH,RXSTART_INIT>>8);	  
	//ERXWRPTH:ERXWRPTL �Ĵ�������Ӳ����FIFO ��
	//���ĸ�λ��д������յ����ֽڡ� ָ����ֻ���ģ��ڳ�
	//�����յ�һ�����ݰ���Ӳ�����Զ�����ָ�롣 ָ���
	//�����ж�FIFO ��ʣ��ռ�Ĵ�С  8K-1500�� 
	//���ý��ն�ָ���ֽ�
	ENC28J60_Write(ERXRDPTL,RXSTART_INIT&0xFF);
	ENC28J60_Write(ERXRDPTH,RXSTART_INIT>>8);
	//���ý��ս����ֽ�
	ENC28J60_Write(ERXNDL,RXSTOP_INIT&0xFF);
	ENC28J60_Write(ERXNDH,RXSTOP_INIT>>8);
	//���÷�����ʼ�ֽ�
	ENC28J60_Write(ETXSTL,TXSTART_INIT&0xFF);
	ENC28J60_Write(ETXSTH,TXSTART_INIT>>8);
	//���÷��ͽ����ֽ�
	ENC28J60_Write(ETXNDL,TXSTOP_INIT&0xFF);
	ENC28J60_Write(ETXNDH,TXSTOP_INIT>>8);
	// do bank 1 stuff,packet filter:
	// For broadcast packets we allow only ARP packtets
	// All other packets should be unicast only for our mac (MAADR)
	//
	// The pattern to match on is therefore
	// Type     ETH.DST
	// ARP      BROADCAST
	// 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
	// in binary these poitions are:11 0000 0011 1111
	// This is hex 303F->EPMM0=0x3f,EPMM1=0x30
	//���չ�����
	//UCEN������������ʹ��λ
	//��ANDOR = 1 ʱ��
	//1 = Ŀ���ַ�뱾��MAC ��ַ��ƥ������ݰ���������
	//0 = ��ֹ������
	//��ANDOR = 0 ʱ��
	//1 = Ŀ���ַ�뱾��MAC ��ַƥ������ݰ��ᱻ����
	//0 = ��ֹ������
	//CRCEN���������CRC У��ʹ��λ
	//1 = ����CRC ��Ч�����ݰ�����������
	//0 = ������CRC �Ƿ���Ч
	//PMEN����ʽƥ�������ʹ��λ
	//��ANDOR = 1 ʱ��
	//1 = ���ݰ�������ϸ�ʽƥ�����������򽫱�����
	//0 = ��ֹ������
	//��ANDOR = 0 ʱ��
	//1 = ���ϸ�ʽƥ�����������ݰ���������
	//0 = ��ֹ������
//	ENC28J60_Write(ERXFCON,ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
//	ENC28J60_Write(ERXFCON,ERXFCON_UCEN /*| ERXFCON_CRCEN *//*| ERXFCON_ANDOR */ | ERXFCON_BCEN);	/* �����������͹㲥�� */
//	ENC28J60_Write(ERXFCON,ERXFCON_CRCEN);	/* ������CRC�����յİ�/s�ͻ���ģʽ��� */
//	ENC28J60_Write(ERXFCON,0);		/* ����ģʽ */
	ENC28J60_Write(ERXFCON,ERXFCON_UCEN | ERXFCON_BCEN | ERXFCON_CRCEN);		/* ����ģʽ */
	
	
	ENC28J60_Write(EPMM0,0x3f);
	ENC28J60_Write(EPMM1,0x30);
	ENC28J60_Write(EPMCSL,0xf9);
	ENC28J60_Write(EPMCSH,0xf7);
	// do bank 2 stuff
	// enable MAC receive
	//bit 0 MARXEN��MAC ����ʹ��λ
	//1 = ����MAC �������ݰ�
	//0 = ��ֹ���ݰ�����
	//bit 3 TXPAUS����ͣ����֡����ʹ��λ
	//1 = ����MAC ������ͣ����֡������ȫ˫��ģʽ�µ��������ƣ�
	//0 = ��ֹ��ͣ֡����
	//bit 2 RXPAUS����ͣ����֡����ʹ��λ
	//1 = �����յ���ͣ����֡ʱ����ֹ���ͣ�����������
	//0 = ���Խ��յ�����ͣ����֡
//	ENC28J60_Write(MACON1,MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);	//ʹ��Ӳ�����أ�TX���ز���Ҫ��Ԥ��RX�����������������룬���ֲ�P58
	ENC28J60_Write(MACON1,MACON1_MARXEN);	//��ֹ����
	// bring MAC out of reset
	//��MACON2 �е�MARST λ���㣬ʹMAC �˳���λ״̬��
	ENC28J60_Write(MACON2,0x00);
	// enable automatic padding to 60bytes and CRC operations
	//bit 7-5 PADCFG2:PACDFG0���Զ�����CRC ����λ
	//111 = ��0 ������ж�֡��64 �ֽڳ�����׷��һ����Ч��CRC
	//110 = ���Զ�����֡
	//101 = MAC �Զ�������8100h �����ֶε�VLAN Э��֡�����Զ���䵽64 �ֽڳ��������
	//��VLAN ֡���������60 �ֽڳ�������Ҫ׷��һ����Ч��CRC
	//100 = ���Զ�����֡
	//011 = ��0 ������ж�֡��64 �ֽڳ�����׷��һ����Ч��CRC
	//010 = ���Զ�����֡
	//001 = ��0 ������ж�֡��60 �ֽڳ�����׷��һ����Ч��CRC
	//000 = ���Զ�����֡
	//bit 4 TXCRCEN������CRC ʹ��λ
	//1 = ����PADCFG��Σ�MAC�����ڷ���֡��ĩβ׷��һ����Ч��CRC�� ���PADCFG�涨Ҫ
	//׷����Ч��CRC������뽫TXCRCEN ��1��
	//0 = MAC����׷��CRC�� ������4 ���ֽڣ����������Ч��CRC �򱨸������״̬������
	//bit 0 FULDPX��MAC ȫ˫��ʹ��λ
	//1 = MAC������ȫ˫��ģʽ�¡� PHCON1.PDPXMD λ������1��
	//0 = MAC�����ڰ�˫��ģʽ�¡� PHCON1.PDPXMD λ�������㡣
	ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,MACON3,MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN|MACON3_FULDPX);
	// set inter-frame gap (non-back-to-back)
	//���÷Ǳ��Ա��������Ĵ����ĵ��ֽ�
	//MAIPGL�� �����Ӧ��ʹ��12h ��̸üĴ�����
	//���ʹ�ð�˫��ģʽ��Ӧ��̷Ǳ��Ա�������
	//�Ĵ����ĸ��ֽ�MAIPGH�� �����Ӧ��ʹ��0Ch
	//��̸üĴ�����
	ENC28J60_Write(MAIPGL,0x12);
	ENC28J60_Write(MAIPGH,0x0C);
	// set inter-frame gap (back-to-back)
	//���ñ��Ա��������Ĵ���MABBIPG����ʹ��
	//ȫ˫��ģʽʱ�������Ӧ��ʹ��15h ��̸üĴ�
	//������ʹ�ð�˫��ģʽʱ��ʹ��12h ���б�̡�
	ENC28J60_Write(MABBIPG,0x15);
	// Set the maximum packet size which the controller will accept
	// Do not send packets longer than MAX_FRAMELEN:
	// ���֡����  1500
	ENC28J60_Write(MAMXFLL,MAX_FRAMELEN&0xFF);	
	ENC28J60_Write(MAMXFLH,MAX_FRAMELEN>>8);
	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	//����MAC��ַ
	ENC28J60_Write(MAADR5,macaddr[0]);	
	ENC28J60_Write(MAADR4,macaddr[1]);
	ENC28J60_Write(MAADR3,macaddr[2]);
	ENC28J60_Write(MAADR2,macaddr[3]);
	ENC28J60_Write(MAADR1,macaddr[4]);
	ENC28J60_Write(MAADR0,macaddr[5]);
	//����PHYΪȫ˫��  LEDBΪ������
	ENC28J60_PHY_Write(PHCON1,PHCON1_PDPXMD);	 
	// no loopback of transmitted frames	 ��ֹ����
	//HDLDIS��PHY ��˫�����ؽ�ֹλ
	//��PHCON1.PDPXMD = 1 ��PHCON1.PLOOPBK = 1 ʱ��
	//��λ�ɱ����ԡ�
	//��PHCON1.PDPXMD = 0 ��PHCON1.PLOOPBK = 0 ʱ��
	//1 = Ҫ���͵����ݽ�ͨ��˫���߽ӿڷ���
	//0 = Ҫ���͵����ݻỷ�ص�MAC ��ͨ��˫���߽ӿڷ���
	ENC28J60_PHY_Write(PHCON2,PHCON2_HDLDIS);
	// switch to bank 0
	//ECON1 �Ĵ���
	//�Ĵ���3-1 ��ʾΪECON1 �Ĵ����������ڿ���
	//ENC28J60 ����Ҫ���ܡ� ECON1 �а�������ʹ�ܡ���
	//������DMA ���ƺʹ洢��ѡ��λ��	   
	ENC28J60_Set_Bank(ECON1);
	// enable interrutps
	//EIE�� ��̫���ж�����Ĵ���
	//bit 7 INTIE�� ȫ��INT �ж�����λ
	//1 = �����ж��¼�����INT ����
	//0 = ��ֹ����INT ���ŵĻ������ʼ�ձ�����Ϊ�ߵ�ƽ��
	//bit 6 PKTIE�� �������ݰ��������ж�����λ
	//1 = ����������ݰ��������ж�
	//0 = ��ֹ�������ݰ��������ж�
//	ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,EIE,EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	//bit 2 RXEN������ʹ��λ
	//1 = ͨ����ǰ�����������ݰ�����д����ջ�����
	//0 = �������н��յ����ݰ�
	ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_RXEN);
	if(ENC28J60_Read(MAADR5)== macaddr[0])
	{
		return 0;//��ʼ���ɹ�
	}
	else 
	{
		return 1; 	  
	}
}

