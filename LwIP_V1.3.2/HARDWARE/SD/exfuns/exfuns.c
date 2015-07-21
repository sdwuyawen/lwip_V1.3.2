#include "string.h"
#include "stdio.h"
#include "exfuns.h"
#include "fattester.h"	
#include "my_malloc.h"
#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//FATFS 扩展代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/18
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

 //文件类型列表
const u8 *FILE_TYPE_TBL[6][13]=
{
{"BIN"},			//BIN文件
{"LRC"},			//LRC文件
{"NES"},			//NES文件
{"TXT","C","H"},	//文本文件
{"MP1","MP2","MP3","MP4","M4A","3GP","3G2","OGG","ACC","WMA","WAV","MID","FLAC"},//音乐文件
{"BMP","JPG","JPEG","GIF"},//图片文件
};
///////////////////////////////公共文件区,使用malloc的时候////////////////////////////////////////////
FATFS *fs[2];  		//逻辑磁盘工作区.	 
FIL *file;	  		//文件1
FIL *ftemp;	  		//文件2.
UINT br,bw;			//读写变量
FILINFO fileinfo;	//文件信息
DIR dir;  			//目录
FRESULT res;         /* 函数返回值存储变量 */ 
char path0[512]="0:";//文件路径
char *Create_File_Name;//新创建的文本文件的名字

u8 *fatbuf;			//SD卡数据缓存区
///////////////////////////////////////////////////////////////////////////////////////
//为exfuns申请内存
//返回值:0,成功
//1,失败
u8 exfuns_init(void)
{
	fs[0]=(FATFS*)mymalloc(SRAMIN,sizeof(FATFS));	//为磁盘0工作区申请内存	
	fs[1]=(FATFS*)mymalloc(SRAMIN,sizeof(FATFS));	//为磁盘1工作区申请内存
	file=(FIL*)mymalloc(SRAMIN,sizeof(FIL));		//为file申请内存
	ftemp=(FIL*)mymalloc(SRAMIN,sizeof(FIL));		//为ftemp申请内存
	fatbuf=(u8*)mymalloc(SRAMIN,512);				//为fatbuf申请内存
	Create_File_Name=(char*)mymalloc(SRAMIN,20);	//为新文件名申请内存
	if(fs[0]&&fs[1]&&file&&ftemp&&fatbuf&&Create_File_Name)//申请有一个失败,即失败.
		return 0;  
	else 
		return 1;	
}

//将小写字母转为大写字母,如果是数字,则保持不变.
u8 char_upper(u8 c)
{
	if(c<'A')return c;//数字,保持不变.
	if(c>='a')return c-0x20;//变为大写.
	else return c;//大写,保持不变
}	      
//报告文件的类型
//fname:文件名
//返回值:0XFF,表示无法识别的文件类型编号.
//		 其他,高四位表示所属大类,低四位表示所属小类.
uint8_t f_typetell(char *fname)
{
	u8 tbuf[5];
	char *attr='\0';//后缀名
	u8 i=0,j;
	while(i<250)
	{
		i++;
		if(*fname=='\0')break;//偏移到了最后了.
		fname++;
	}
	if(i==250)return 0XFF;//错误的字符串.
 	for(i=0;i<5;i++)//得到后缀名
	{
		fname--;
		if(*fname=='.')
		{
			fname++;
			attr=fname;
			break;
		}
  	}
	strcpy((char *)tbuf,(const char*)attr);//copy
 	for(i=0;i<4;i++)tbuf[i]=char_upper(tbuf[i]);//全部变为大写 
	for(i=0;i<6;i++)
	{
		for(j=0;j<13;j++)
		{
			if(*FILE_TYPE_TBL[i][j]==0)break;//此组已经没有可对比的成员了.
			if(strcmp((const char *)FILE_TYPE_TBL[i][j],(const char *)tbuf)==0)//找到了
			{
				return (i<<4)|j;
			}
		}
	}
	return 0XFF;//没找到		 			   
}	 

//得到磁盘剩余容量
//drv:磁盘编号("0:"/"1:")
//total:总容量	 （单位KB）
//free:剩余容量	 （单位KB）
//返回值:0,正常.其他,错误代码
u8 exf_getfree(u8 *drv,u32 *total,u32 *free)
{
	FATFS *fs1;
	u8 res;
    DWORD fre_clust=0, fre_sect=0, tot_sect=0;
    //得到磁盘信息及空闲簇数量
    res = f_getfree((const TCHAR*)drv, &fre_clust, &fs1);
    if(res==0)
	{											   
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//得到总扇区数
	    fre_sect=fre_clust*fs1->csize;			//得到空闲扇区数	   
#if _MAX_SS!=512				  				//扇区大小不是512字节,则转换为512字节
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif	  
		*total=tot_sect>>1;	//单位为KB
		*free=fre_sect>>1;	//单位为KB 
 	}
	return res;
}		   


/*******************************************************************************
  * @函数名称	scan_files
  * @函数说明   搜索文件目录下所有文件 
  * @输入参数   path: 根目录 
  * @输出参数   无
  * @返回参数   FRESULT
  * @注意事项	无
  *****************************************************************************/
FRESULT scan_files (char* path,uint8_t f_type,uint16_t *latest_num)       /* Start node to be scanned (also used as work area) */
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    char *fn;   /* This function is assuming non-Unicode cfg. */
#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) 
	{
        i = strlen(path);
        for (;;) 
		{
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) 		   
				break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if (fno.fattrib & AM_DIR)//如果当前项不是文件，而是一个目录
			{                    /* It is a directory */
                sprintf(&path[i], "/%s", fn);//把fn按%s格式输出到path[i]开始的数组，即在当前路径下增加下一级路径
				printf("scan dir - %s\r\n",path);
                res = scan_files(path,f_type,latest_num);//注意这里的递归调用
                if (res != FR_OK) break;
                path[i] = 0;//下一级目录扫描完成，恢复路径为当前路径
            } 
			else 
			{                                       /* It is a file. */
                printf("scan file - %s/%s\r\n", path, fn);//path是"0:"，fn是指向"text2.txt"的指针
// 				printf("文件类型是%x\r\n",f_typetell(fn));
				if(f_typetell(fn)==f_type)//指定类型文件，则搜索最大3位数编号
				{
					File_Latest_Name(fn,latest_num);
				}
// 				printf("当前最大序号是%d\r\n",*latest_num);
            }
        }
    }
	else
	{
		printf("scan files error : %d\r\n",res);
	}

    return res;
}

/*******************************************************************************
  * @函数名称	SD_TotalSize
  * @函数说明   文件空间占用情况 
  * @输入参数   无 
  * @输出参数   无
  * @返回参数   1: 成功 
  				0: 失败
  * @注意事项	无
  *****************************************************************************/
int SD_TotalSize(char *path)
{
    FATFS *fs;
    DWORD fre_clust;        

    res = f_getfree(path, &fre_clust, &fs);  /* 必须是根目录，选择磁盘0 */
    if ( res==FR_OK ) 
    {
	  printf("get %s drive space.\r\n",path);
	  /* Print free space in unit of MB (assuming 512 bytes/sector) */
      printf("%d MB total drive space.\r\n"
           "%d MB available.\r\n",
           ( (fs->n_fatent - 2) * fs->csize ) / 2 /1024 , (fre_clust * fs->csize) / 2 /1024 );

			//(fs->n_fatent - 2) * fs->csize是总扇区数，除以2是KB数，再除以1024是MB数（每个扇区是512bytes）。
	  return 1;
	}
	else
	{ 
	  printf("Get total drive space faild!\r\n");
	  return 0;   
	}
}

//找到指定名字编号最靠后的文件的编号，要求文件名为NAMExxx.zzz，xxx为三位数字，NAME最长为10字节
//fname指向当前文件名
//latest_num指向当前最大的编号
//返回值 0，找到更大的编号；1，没有找到更大的编号；1，文件名序号不是3位
uint8_t File_Latest_Name(char *fname,uint16_t *latest_num)
{
	u8 i=0;
	uint16_t file_num=0;
 	for(i=0;i<10;i++)//找到.
	{
		fname++;
		if(*fname=='.')
		{
			fname-=3;//移到数字位百位
			break;
		}
  	}
	if(*fname>=0x30&&*(fname+1)>=0x30&&*(fname+2)>=0x30&&*fname<=0x39&&*(fname+1)<=0x39&&*(fname+2)<=0x39)//文件名最后3位都是数字
	{
		file_num=((*fname)-0x30)*100+(*(fname+1)-0x30)*10+*(fname+2)-0x30;//得到当前文件的编号
		if(*latest_num<file_num) //当前文件编号大
		{
			*latest_num=file_num;
			return 0;
		}	
		return 1;
	}
	return 2;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////




















