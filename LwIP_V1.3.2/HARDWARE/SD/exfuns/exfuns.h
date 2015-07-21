#ifndef __EXFUNS_H
#define __EXFUNS_H 	
#include "sys_m3.h"		   
#include "ff.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////
extern FATFS *fs[2];  
extern FIL *file;	 
extern FIL *ftemp;	 
extern UINT br,bw;
extern FILINFO fileinfo;
extern DIR dir;
extern u8 *fatbuf;//SD�����ݻ�����
extern FRESULT res;         /* ��������ֵ�洢���� */
extern char path0[512];//�ļ�·��
extern char *Create_File_Name;//�´������ı��ļ�������


//f_typetell���ص����Ͷ���
//���ݱ�FILE_TYPE_TBL���.��exfuns.c���涨��
#define T_BIN		0X00	//bin�ļ�
#define T_LRC		0X10	//lrc�ļ�
#define T_NES		0X20	//nes�ļ�
#define T_TEXT		0X30	//.txt�ļ�
#define T_C			0X31	//.c�ļ�
#define T_H			0X32    //.h�ļ�
#define T_FLAC		0X4C	//flac�ļ�
#define T_BMP		0X50	//bmp�ļ�
#define T_JPG		0X51	//jpg�ļ�
#define T_JPEG		0X52	//jpeg�ļ�		 
#define T_GIF		0X53	//gif�ļ�  

 
u8 exfuns_init(void);		//�����ڴ�
uint8_t f_typetell(char *fname);	//ʶ���ļ�����
u8 exf_getfree(u8 *drv,u32 *total,u32 *free);//�õ�������������ʣ������
u32 exf_fdsize(u8 *fdname);																				   //�õ��ļ��д�С
u8* exf_get_src_dname(u8* dpfn);																		   
u8 exf_copy(u8(*fcpymsg)(u8*pname,u8 pct,u8 mode),u8 *psrc,u8 *pdst,u32 totsize,u32 cpdsize,u8 fwmode);	   //�ļ�����
u8 exf_fdcopy(u8(*fcpymsg)(u8*pname,u8 pct,u8 mode),u8 *psrc,u8 *pdst,u32 *totsize,u32 *cpdsize,u8 fwmode);//�ļ��и���

FRESULT scan_files (char* path,uint8_t f_type,uint16_t *latest_num); //�����ļ�Ŀ¼�������ļ� 
int SD_TotalSize(char *path);//�ļ��ռ�ռ����� 
uint8_t File_Latest_Name(char *fname,uint16_t *latest_num);//��������ļ����

#endif


