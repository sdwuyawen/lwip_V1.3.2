#include "ssi_cgi_handle.h"
#include "httpd.h"
#include "string.h"
#include "usart.h"
#include "stdio.h"
#include "fs.h"

#include "led.h"
#include "rtc.h" 
#include "lcd.h"
#include "adc.h"
#include "tsensor.h"

#define INDEX_PAGE_SET_CGI_RSP_URL        "/index.html"

#define RESPONSE_PAGE_SET_CGI_RSP_URL     "/response.ssi"


#define NUM_CONFIG_CGI_URIS     (sizeof(ppcURLs ) / sizeof(tCGI))
#define NUM_CONFIG_SSI_TAGS     (sizeof(ppcTags) / sizeof (char *))


static char *LED_RED_CGIHandler( int iIndex, int iNumParams, char *pcParam[], char *pcValue[] );
static char *LED_GREEN_CGIHandler( int iIndex, int iNumParams, char *pcParam[], char *pcValue[] );
static char *Orther_CGIHandler( int iIndex, int iNumParams, char *pcParam[], char *pcValue[] );


static int SSIHandler ( int iIndex, char *pcInsert, int iInsertLen );


// typedef struct{
//  unsigned char *bufptr;   //指向响应缓冲区的指针
//  int buf_user;   //指向缓冲区的使用情况
// }RESPONSE_BUF;

// RESPONSE_BUF  response_buf;

static const tCGI ppcURLs[] =
{

    { "/led_red.cgi",      LED_RED_CGIHandler },  
    { "/led_green.cgi",    LED_GREEN_CGIHandler },    
    { "/orther.cgi",       Orther_CGIHandler },     
};

static const char *ppcTags[] =
{
    "onetree",
	  "filest"
};

enum ssi_index_s
{
    SSI_INDEX_ONETREE_GET = 0, //该表对应ppcTags[]的排序
    SSI_INDEX_FILEST_GET

} ;



//数字->字符串转换函数
//将num数字(位数为len)转为字符串,存放在buf里面
//num:数字,整形
//buf:字符串缓存
//len:长度
void num2str(u16 num,u8 *buf,u8 len)
{
	u8 i;
	for(i=0;i<len;i++)
	{
		buf[i]=(num/LCD_Pow(10,len-i-1))%10+'0';
	}
}
//获取STM32内部温度传感器的温度
//temp:存放温度字符串的首地址.如"28.3";
//temp,最少得有5个字节的空间!
void get_temperature(u8 *temp)
{			  
	u16 t;
	float temperate;		   
	temperate=Get_Adc_Average(ADC_CH_TEMP,10);			 
	temperate=temperate*(3.3/4096);			    											    
	temperate=(1.43-temperate)/0.0043+25;	//计算出当前温度值
	t=temperate*10;//得到温度
	num2str(t/10,temp,2);							   
	temp[2]='.';temp[3]=t%10+'0';temp[4]=0;	//最后添加结束符
}
//获取RTC时间
//time:存放时间字符串,形如:"2012-09-27 12:33:00"
//time,最少得有17个字节的空间!
void get_time(u8 *time)
{	
	RTC_Get();
	time[4]='-';time[7]='-';time[10]=' ';
	time[13]=':';time[16]=':';time[19]=0;			//最后添加结束符
	num2str(calendar.w_year,time,4);	//年份->字符串
	num2str(calendar.w_month,time+5,2); //月份->字符串	 
	num2str(calendar.w_date,time+8,2); 	//日期->字符串
	num2str(calendar.hour,time+11,2); 	//小时->字符串
	num2str(calendar.min,time+14,2); 	//分钟->字符串		
  num2str(calendar.sec,time+17,2); 	//秒->字符串		
}

//初始化ssi和cgi
void init_ssi_cgi(void){
	
 http_set_cgi_handlers(ppcURLs , NUM_CONFIG_CGI_URIS);	
 http_set_ssi_handler (SSIHandler, ppcTags, NUM_CONFIG_SSI_TAGS );


}

//*****************************************************************************
//
// This CGI handler is called whenever the web browser requests iocontrol.cgi.
//
//*****************************************************************************
static int FindCGIParameter(const char *pcToFind, char *pcParam[], int iNumParams)
{
    int iLoop;

    for(iLoop = 0; iLoop < iNumParams; iLoop++)
    {
        if(strcmp(pcToFind, pcParam[iLoop]) == 0)
        {
            return(iLoop);
        }
    }

    return(-1);
}

//清除缓冲区的内容
void  clear_response_bufer(unsigned char *buffer){
  memset(buffer,0,strlen((const char*)buffer));
}
int num=100;

//红灯处理函数
static char *LED_RED_CGIHandler( int iIndex, int iNumParams, char *pcParam[], char *pcValue[] )
{
    int  index;
    index = FindCGIParameter ( "red", pcParam, iNumParams );
    if(index != -1)
    {
			clear_response_bufer(data_response_buf);      //清除缓冲区的内容
			LED0 = !LED0;
			if(!LED0){
					strcat((char *)(data_response_buf),"/img/red.gif");
      }else{
          strcat((char *)(data_response_buf),"/img/black.gif");
      }
        
    }
    return RESPONSE_PAGE_SET_CGI_RSP_URL;
}


//绿灯处理函数
static char *LED_GREEN_CGIHandler( int iIndex, int iNumParams, char *pcParam[], char *pcValue[] )
{
    int  index;
    index = FindCGIParameter ( "green", pcParam, iNumParams );
    if(index != -1)
    {
			// num++;
			 //snprintf((char *)(data_response_buf),4,"%d",num);
			// printf("data_response_buf:%s\r\n",data_response_buf);
			 //printf("green:%s\r\n",pcValue[index]);	
			
			 clear_response_bufer(data_response_buf);      //清除缓冲区的内容
       LED1 = !LED1;
			 if(!LED1){
					strcat((char *)(data_response_buf),"/img/green.gif");
       }else{
          strcat((char *)(data_response_buf),"/img/black.gif");
       }
        
    }
    return RESPONSE_PAGE_SET_CGI_RSP_URL;
}


//绿灯处理函数
static char *Orther_CGIHandler( int iIndex, int iNumParams, char *pcParam[], char *pcValue[] )
{
    u8 buf[20];
    clear_response_bufer(data_response_buf);      //清除缓冲区的内容

	  get_temperature(data_response_buf);
	  strcat((char *)(data_response_buf),";");
	  get_time(buf);
	  strcat((char *)(data_response_buf),buf);
    return RESPONSE_PAGE_SET_CGI_RSP_URL;
}


//*****************************************************************************
//
// This function is called by the HTTP server whenever it encounters an SSI
// tag in a web page.  The iIndex parameter provides the index of the tag in
// the ppcTags array. This function writes the substitution text
// into the pcInsert array, writing no more than iInsertLen characters.
//
//*****************************************************************************
static int SSIHandler ( int iIndex, char *pcInsert, int iInsertLen )
{   
	
	 
    switch(iIndex)
    {
        case SSI_INDEX_ONETREE_GET:

            break;
            
        case SSI_INDEX_FILEST_GET:
            
            break;       
        default:
            strcpy( pcInsert , "??" );
            
    }
 
    return strlen ( pcInsert );
}


