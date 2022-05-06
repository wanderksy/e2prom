/* bps_emu_e2prom.c
   * Date                 author		Notes
 * 2022-04-14             willow		first version
*/

#ifndef __EMU_E2PROM_H
#define __EMU_E2PROM_H

#include <stdint.h>

//#define		BASS_ADDR_EMU_FLASH		ADDR_FLASH_SECTOR_11

#define		SUM_ELEMENT			(3U)	//ģ���Ԫ���ܸ���


//Ԫ�ض��� ����STM32H7 ֱ�Ӷ���32Byte (��������32����������)
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


