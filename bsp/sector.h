#ifndef __SECTOR_H
#define __SECTOR_H

#define MCU_FLASH_SIZE_1MB	0
#define MCU_FLASH_SIZE_2MB	1


#define INVALID_RESULT 0xFFFFFFFF
#define INITIAL_SECTOR_INDEX 0	// 假设扇区是从0开始编号的 sector2
#define SECTOR_INDEX_SCALE	1//8	//stm32 的扇区编号是以8为单位的 0 0x08 0x10 0x18 0x20

typedef const struct
{
	unsigned char 	sec_num; 	/* sector number of the sector group */
	unsigned char 	sec_size;    /* each sector size in KB */
	unsigned char 	sec_index;	/* starting index of the sector group */
} SECTOR_DESC_T;

unsigned long getSectorIndex(unsigned long addr);
unsigned long getSectorSize(unsigned long sectorIndex);
unsigned long getEndSectorIndex(unsigned long sizeKB, unsigned long startIndex);
unsigned long getMaxSectorIndex(void);
unsigned char is_starting_addressofsector(unsigned long addr);		//willow add

#endif
