/*
*********************************************************************************************************
*
*	ģ������ : BSPģ��
*	�ļ����� : bsp.h
*	˵    �� : ���ǵײ�����ģ�����е�h�ļ��Ļ����ļ��� Ӧ�ó���ֻ�� #include bsp.h ���ɣ�
*			  ����Ҫ#include ÿ��ģ��� h �ļ�
*
*	Copyright (C), 2013-2014, ���������� www.armfly.com
*
*********************************************************************************************************
*/

#ifndef _BSP_H_
#define _BSP_H

#include "stm32f10x.h"

//#define STM32_V5
//#define STM32_X4

/* ����Ƿ����˿������ͺ� */
// #if !defined (STM32_V5) && !defined (STM32_X4)
// 	#error "Please define the board model : STM32_X4 or STM32_V5"
// #endif

/* ���� BSP �汾�� */
#define __STM32F1_BSP_VERSION		"1.1"

/* ����ȫ���жϵĺ� */
#define ENABLE_INT()	__set_PRIMASK(0)	/* ʹ��ȫ���ж� */
#define DISABLE_INT()	__set_PRIMASK(1)	/* ��ֹȫ���ж� */

/* ���������ڵ��Խ׶��Ŵ� */
#define BSP_Printf		printf
//#define BSP_Printf(...)

#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

#ifndef TRUE
	#define TRUE  1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

/* ͨ��ȡ��ע�ͻ������ע�͵ķ�ʽ�����Ƿ�����ײ�����ģ�� */
// #include "bsp_uart_fifo.h"
// #include "bsp_led.h"
#include "bsp_ticktimer.h"
#include "bsp_key.h"

// #include "bsp_spi_flash.h"
//#include "bsp_cpu_flash.h"
//#include "bsp_sdio_sd.h"
//#include "bsp_i2c_gpio.h"
//#include "bsp_eeprom_24xx.h"
//#include "bsp_si4730.h"
//#include "bsp_hmc5883l.h"
//#include "bsp_mpu6050.h"
//#include "bsp_bh1750.h"
//#include "bsp_bmp085.h"
//#include "bsp_wm8978.h"
//#include "bsp_fsmc_sram.h"
//#include "bsp_nand_flash.h"
//#include "bsp_nor_flash.h"
//#include "LCD_RA8875.h"
//#include "LCD_SPFD5420.h"
//#include "bsp_touch.h"
//#include "bsp_camera.h"
//#include "bsp_ad7606.h"
//#include "bsp_gps.h"
//#include "bsp_oled.h"
//#include "bsp_mg323.h"

/* �ṩ������C�ļ����õĺ��� */
void bsp_Init(void);
void bsp_Idle(void);

#endif

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
