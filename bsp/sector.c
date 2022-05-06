/* sector.c
   * Date                 author		Notes
 * 2021-07-29             willow		¼æÈÝH743
  ******************************************************************************/
#include "stm32h7xx_hal.h"
#include "sector.h"
#include "flash_if.h"

/* internal flash sectors definition, 
	the last SECTOR_DESC_T.sec_num should be zero */
SECTOR_DESC_T sector_desc[] =  	
#if MCU_FLASH_SIZE_1MB
{
	{4,  16,  0}, 	// 4 16KB sectors, index: 0..3	   	//1M Byte flash
	{1,  64,  4},	// 1 64KB sectors, index: 4
	{7,  128, 5},	// 7 128KB sectors, index: 5..12
	{0,  0,  0}		// should be zero
}; 
#elif MCU_FLASH_SIZE_2MB

	#if defined(STM32H743xx)
	{
		{16,  128,  0}, 	// 4 128KB sectors, index: 0..7	   	//stm32h743 2M Byte flash
		{0,  0,  0}		// should be zero
	};
	#else
	{
		{4,  16,  0}, 	// 4 16KB sectors, index: 0..3	   	//1M Byte flash
		{1,  64,  4},	// 1 64KB sectors, index: 4
		{7,  128, 5},	// 7 128KB sectors, index: 5..12
		{4,  16,  12}, 	// 4 16KB sectors, index: 0..3	   	//1M Byte flash
		{1,  64,  16},	// 1 64KB sectors, index: 4
		{7,  128, 17},	// 7 128KB sectors, index: 5..12
		{0,  0,  0}		// should be zero
	};
	#endif
#endif
	
#if 1
unsigned long getSectorIndex(unsigned long addr)
{
	SECTOR_DESC_T *psg = &sector_desc[0];
	unsigned long tmp, size_acc, sector_index, size_addr;

	size_acc = 0;
	//size_addr = addr>>10;
	size_addr = (addr-FLASH_BASE)>>10;
	sector_index = INITIAL_SECTOR_INDEX; 
	while (psg->sec_num)
	{
		tmp = size_addr - size_acc;  //KB
		if (psg->sec_size*psg->sec_num > tmp)
		{
			sector_index += tmp/psg->sec_size;
			return sector_index*SECTOR_INDEX_SCALE;	
		}
		else
		{
			sector_index += psg->sec_num;
			size_acc += psg->sec_size*psg->sec_num;
		}
		psg++;
	}
	
	return INVALID_RESULT;	
}
#endif

/* Return the size (in KB) of specified sector (sectorIndex),
	if failed, return INVALID_RESULT. */
unsigned long getSectorSize(unsigned long sectorIndex)
{
	SECTOR_DESC_T *psg = &sector_desc[0];
	unsigned long index;

	index =  sectorIndex;
	while (psg->sec_num)
	{
		if (index < psg->sec_num)
			return psg->sec_size;
		index -= psg->sec_num;
		psg++;
	}
	return INVALID_RESULT;		
}

/* addr is or not a starting address of sector?
 return 1 is starting address
 		0  not starting address  */
unsigned char is_starting_addressofsector(unsigned long addr)
{
	SECTOR_DESC_T *psg = &sector_desc[0];
	uint32_t	temp=0;
	uint32_t	i;

	temp=ADDR_FLASH_SECTOR_0_BANK1;//ADDR_FLASH_SECTOR_0;	//flash bass address
	while (psg->sec_num)
	{
		for(i=0;i<psg->sec_num;i++)
		{
			temp+=((psg->sec_size)<<10);
			if(temp==addr)
				return	1;		
		}
		psg++;
	}
	return 0;		
}

/* Return the end sector index according to the start sector index and image size (in Byte),
	if failed , return INVALID_RESULT */
unsigned long getEndSectorIndex(unsigned long size, unsigned long startIndex)
{
	unsigned int index, sec_size_KB, size_tmp;

	if (size == 0)
		return INVALID_RESULT;

	index = startIndex;
	size_tmp = 0;	

	while (size_tmp < size)
	{
		sec_size_KB = getSectorSize(index);	 // in KB
		if (sec_size_KB == INVALID_RESULT)
			return INVALID_RESULT;

		size_tmp +=	(sec_size_KB<<10);
		index++;
	}
	return (index-1);	
}

/* return the max sector index,
	it is supposed that the sector index starts from 0 */
unsigned long getMaxSectorIndex(void)
{
	SECTOR_DESC_T *psg = &sector_desc[0];
	unsigned long index_sum = 0;

	while (psg->sec_num)
	{
		index_sum += psg->sec_num;	
	}
	
	return (index_sum-1);	
}

