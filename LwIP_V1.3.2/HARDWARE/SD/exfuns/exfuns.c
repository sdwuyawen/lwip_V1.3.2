#include "string.h"
#include "stdio.h"
#include "exfuns.h"
#include "fattester.h"	
#include "my_malloc.h"
#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������
//FATFS ��չ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/18
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

 //�ļ������б�
const u8 *FILE_TYPE_TBL[6][13]=
{
{"BIN"},			//BIN�ļ�
{"LRC"},			//LRC�ļ�
{"NES"},			//NES�ļ�
{"TXT","C","H"},	//�ı��ļ�
{"MP1","MP2","MP3","MP4","M4A","3GP","3G2","OGG","ACC","WMA","WAV","MID","FLAC"},//�����ļ�
{"BMP","JPG","JPEG","GIF"},//ͼƬ�ļ�
};
///////////////////////////////�����ļ���,ʹ��malloc��ʱ��////////////////////////////////////////////
FATFS *fs[2];  		//�߼����̹�����.	 
FIL *file;	  		//�ļ�1
FIL *ftemp;	  		//�ļ�2.
UINT br,bw;			//��д����
FILINFO fileinfo;	//�ļ���Ϣ
DIR dir;  			//Ŀ¼
FRESULT res;         /* ��������ֵ�洢���� */ 
char path0[512]="0:";//�ļ�·��
char *Create_File_Name;//�´������ı��ļ�������

u8 *fatbuf;			//SD�����ݻ�����
///////////////////////////////////////////////////////////////////////////////////////
//Ϊexfuns�����ڴ�
//����ֵ:0,�ɹ�
//1,ʧ��
u8 exfuns_init(void)
{
	fs[0]=(FATFS*)mymalloc(SRAMIN,sizeof(FATFS));	//Ϊ����0�����������ڴ�	
	fs[1]=(FATFS*)mymalloc(SRAMIN,sizeof(FATFS));	//Ϊ����1�����������ڴ�
	file=(FIL*)mymalloc(SRAMIN,sizeof(FIL));		//Ϊfile�����ڴ�
	ftemp=(FIL*)mymalloc(SRAMIN,sizeof(FIL));		//Ϊftemp�����ڴ�
	fatbuf=(u8*)mymalloc(SRAMIN,512);				//Ϊfatbuf�����ڴ�
	Create_File_Name=(char*)mymalloc(SRAMIN,20);	//Ϊ���ļ��������ڴ�
	if(fs[0]&&fs[1]&&file&&ftemp&&fatbuf&&Create_File_Name)//������һ��ʧ��,��ʧ��.
		return 0;  
	else 
		return 1;	
}

//��Сд��ĸתΪ��д��ĸ,���������,�򱣳ֲ���.
u8 char_upper(u8 c)
{
	if(c<'A')return c;//����,���ֲ���.
	if(c>='a')return c-0x20;//��Ϊ��д.
	else return c;//��д,���ֲ���
}	      
//�����ļ�������
//fname:�ļ���
//����ֵ:0XFF,��ʾ�޷�ʶ����ļ����ͱ��.
//		 ����,����λ��ʾ��������,����λ��ʾ����С��.
uint8_t f_typetell(char *fname)
{
	u8 tbuf[5];
	char *attr='\0';//��׺��
	u8 i=0,j;
	while(i<250)
	{
		i++;
		if(*fname=='\0')break;//ƫ�Ƶ��������.
		fname++;
	}
	if(i==250)return 0XFF;//������ַ���.
 	for(i=0;i<5;i++)//�õ���׺��
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
 	for(i=0;i<4;i++)tbuf[i]=char_upper(tbuf[i]);//ȫ����Ϊ��д 
	for(i=0;i<6;i++)
	{
		for(j=0;j<13;j++)
		{
			if(*FILE_TYPE_TBL[i][j]==0)break;//�����Ѿ�û�пɶԱȵĳ�Ա��.
			if(strcmp((const char *)FILE_TYPE_TBL[i][j],(const char *)tbuf)==0)//�ҵ���
			{
				return (i<<4)|j;
			}
		}
	}
	return 0XFF;//û�ҵ�		 			   
}	 

//�õ�����ʣ������
//drv:���̱��("0:"/"1:")
//total:������	 ����λKB��
//free:ʣ������	 ����λKB��
//����ֵ:0,����.����,�������
u8 exf_getfree(u8 *drv,u32 *total,u32 *free)
{
	FATFS *fs1;
	u8 res;
    DWORD fre_clust=0, fre_sect=0, tot_sect=0;
    //�õ�������Ϣ�����д�����
    res = f_getfree((const TCHAR*)drv, &fre_clust, &fs1);
    if(res==0)
	{											   
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//�õ���������
	    fre_sect=fre_clust*fs1->csize;			//�õ�����������	   
#if _MAX_SS!=512				  				//������С����512�ֽ�,��ת��Ϊ512�ֽ�
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif	  
		*total=tot_sect>>1;	//��λΪKB
		*free=fre_sect>>1;	//��λΪKB 
 	}
	return res;
}		   


/*******************************************************************************
  * @��������	scan_files
  * @����˵��   �����ļ�Ŀ¼�������ļ� 
  * @�������   path: ��Ŀ¼ 
  * @�������   ��
  * @���ز���   FRESULT
  * @ע������	��
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
            if (fno.fattrib & AM_DIR)//�����ǰ����ļ�������һ��Ŀ¼
			{                    /* It is a directory */
                sprintf(&path[i], "/%s", fn);//��fn��%s��ʽ�����path[i]��ʼ�����飬���ڵ�ǰ·����������һ��·��
				printf("scan dir - %s\r\n",path);
                res = scan_files(path,f_type,latest_num);//ע������ĵݹ����
                if (res != FR_OK) break;
                path[i] = 0;//��һ��Ŀ¼ɨ����ɣ��ָ�·��Ϊ��ǰ·��
            } 
			else 
			{                                       /* It is a file. */
                printf("scan file - %s/%s\r\n", path, fn);//path��"0:"��fn��ָ��"text2.txt"��ָ��
// 				printf("�ļ�������%x\r\n",f_typetell(fn));
				if(f_typetell(fn)==f_type)//ָ�������ļ������������3λ�����
				{
					File_Latest_Name(fn,latest_num);
				}
// 				printf("��ǰ��������%d\r\n",*latest_num);
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
  * @��������	SD_TotalSize
  * @����˵��   �ļ��ռ�ռ����� 
  * @�������   �� 
  * @�������   ��
  * @���ز���   1: �ɹ� 
  				0: ʧ��
  * @ע������	��
  *****************************************************************************/
int SD_TotalSize(char *path)
{
    FATFS *fs;
    DWORD fre_clust;        

    res = f_getfree(path, &fre_clust, &fs);  /* �����Ǹ�Ŀ¼��ѡ�����0 */
    if ( res==FR_OK ) 
    {
	  printf("get %s drive space.\r\n",path);
	  /* Print free space in unit of MB (assuming 512 bytes/sector) */
      printf("%d MB total drive space.\r\n"
           "%d MB available.\r\n",
           ( (fs->n_fatent - 2) * fs->csize ) / 2 /1024 , (fre_clust * fs->csize) / 2 /1024 );

			//(fs->n_fatent - 2) * fs->csize����������������2��KB�����ٳ���1024��MB����ÿ��������512bytes����
	  return 1;
	}
	else
	{ 
	  printf("Get total drive space faild!\r\n");
	  return 0;   
	}
}

//�ҵ�ָ�����ֱ�������ļ��ı�ţ�Ҫ���ļ���ΪNAMExxx.zzz��xxxΪ��λ���֣�NAME�Ϊ10�ֽ�
//fnameָ��ǰ�ļ���
//latest_numָ��ǰ���ı��
//����ֵ 0���ҵ�����ı�ţ�1��û���ҵ�����ı�ţ�1���ļ�����Ų���3λ
uint8_t File_Latest_Name(char *fname,uint16_t *latest_num)
{
	u8 i=0;
	uint16_t file_num=0;
 	for(i=0;i<10;i++)//�ҵ�.
	{
		fname++;
		if(*fname=='.')
		{
			fname-=3;//�Ƶ�����λ��λ
			break;
		}
  	}
	if(*fname>=0x30&&*(fname+1)>=0x30&&*(fname+2)>=0x30&&*fname<=0x39&&*(fname+1)<=0x39&&*(fname+2)<=0x39)//�ļ������3λ��������
	{
		file_num=((*fname)-0x30)*100+(*(fname+1)-0x30)*10+*(fname+2)-0x30;//�õ���ǰ�ļ��ı��
		if(*latest_num<file_num) //��ǰ�ļ���Ŵ�
		{
			*latest_num=file_num;
			return 0;
		}	
		return 1;
	}
	return 2;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////




















