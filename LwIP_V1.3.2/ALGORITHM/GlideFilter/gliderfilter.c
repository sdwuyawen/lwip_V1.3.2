#include "stm32f10x.h"
#include "gliderfilter.h"


//滑动平均滤波算法（递推平均滤波法）--C语言版 
//value_buf是历史数据
//Current_Num是当前采集到第几个数据
//Total_Num是最大数据个数
//ADValue为获得的AD数
//n为数组value_buf[]的元素个数。该函数主要被调用，利用参数的数组传值
uint16_t GlideFilterAD(uint16_t value_buf[],uint16_t *Current_Num,uint16_t Total_Num,uint16_t ADValue)
{
	uint16_t sum=0;
	uint16_t count;
	
	value_buf[(*Current_Num)++]=ADValue;
	if(*Current_Num==Total_Num)
		*Current_Num=0; //先进先出，再求平均值
	for(count=0;count<Total_Num;count++)
		sum+=value_buf[count];
	return (sum/Total_Num);
}


