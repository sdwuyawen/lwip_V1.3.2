#include "stm32f10x.h"
#include "gliderfilter.h"


//����ƽ���˲��㷨������ƽ���˲�����--C���԰� 
//value_buf����ʷ����
//Current_Num�ǵ�ǰ�ɼ����ڼ�������
//Total_Num��������ݸ���
//ADValueΪ��õ�AD��
//nΪ����value_buf[]��Ԫ�ظ������ú�����Ҫ�����ã����ò��������鴫ֵ
uint16_t GlideFilterAD(uint16_t value_buf[],uint16_t *Current_Num,uint16_t Total_Num,uint16_t ADValue)
{
	uint16_t sum=0;
	uint16_t count;
	
	value_buf[(*Current_Num)++]=ADValue;
	if(*Current_Num==Total_Num)
		*Current_Num=0; //�Ƚ��ȳ�������ƽ��ֵ
	for(count=0;count<Total_Num;count++)
		sum+=value_buf[count];
	return (sum/Total_Num);
}


