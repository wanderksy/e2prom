/* bps_emu_e2prom.c
   * Date                 author		Notes
 * 2022-04-14             willow		first version
*/

#ifndef __EMU_E2PROM_H
#define __EMU_E2PROM_H

#include <stdint.h>

//#define		BASS_ADDR_EMU_FLASH		ADDR_FLASH_SECTOR_11

#define		SUM_ELEMENT			(3U)	//模拟的元素总个数


//元素定义 兼容STM32H7 直接定义32Byte (可以增大到32的整倍数！)
typedef		struct element  STR_ELEMENT,*P_STR_ELEMENT;
struct element{
	uint16_t addr;
	uint8_t dat[30];
	//STR_ELEMENT * p;//P_STR_ELEMENT p;
};


void e2prom_init(void);
int e2prom_wr_ele(P_STR_ELEMENT pdat);
int e2prom_rd_ele(P_STR_ELEMENT pdat);



#endif


