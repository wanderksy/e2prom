/* bps_emu_e2prom.c
使用内部FLASH的最后一个扇区来模拟E2PROM来保存经常改变的参数。
此参数以参数结构体的形式保存在flash中
struct element{
	uint16_t addr;
	uint8_t dat[30];
};
其中addr为参数地址标示，用来区别不同的参数。
dat[30]，为有效参数空间，最大可以保存30B，如果需要扩展，可以把此结构体的扩展到32B的整数倍即可，为了兼容STM32H7系统内部FLASH。
使用说明：
1. 对BASS_ADDR_EMU_FLASH首地址做定义(不定义则使用最后一个扇区作为模拟FLASH)
2. 上电后先初始化，e2prom_init()
3. 初始化后就可以读写要保存的参数了。
4. 寿命：如果只有1个参数， 128k/32*10000=4000W次,能满足大多数需求

   * Date                 author		Notes
 * 2022-04-14             willow.J		first version
*/
#include <string.h>
#include "stm32h7xx_hal.h"
#include "flash_if.h"
//#include "sector.h"
#include "bsp_emu_e2prom.h"

//#define		BASS_ADDR_EMU_FLASH		ADDR_FLASH_SECTOR_11
////#define		SECTOR_EMU_FLASH		FLASH_SECTOR_11
//#define		SUM_ELEMENT			(10U)	//最多模拟的元素个数

#ifndef STM32H743xx
#define FLASHSIZE_BASE               0x1FFF7A22UL           /*!< FLASH Size register base address       */
#endif

STR_ELEMENT PARE[SUM_ELEMENT];
//uint8_t * p_addr;
uint32_t addr_null;		//即将写入的一个空存储单元地址
uint32_t addr_start;	//模拟E2PROM的开始地址

int LL_wr_flash(P_STR_ELEMENT pdat);
int find_ele_in_ram(uint16_t addr);
int wr_ele_in_ram(P_STR_ELEMENT pdat);
uint32_t get_size_emu_sector(void);


/*
初始化E2PROM
*/
#if 1
void e2prom_init()
{
	uint32_t  addr_flash;
	uint32_t idx;
	uint32_t size_flash;
	
	memset(PARE, 0xFF, sizeof(PARE));	//清空FF参数表
	
	size_flash = (*(uint16_t*)FLASHSIZE_BASE) << 10;
	addr_null = FLASH_BASE + size_flash;
	addr_start = FLASH_BASE + size_flash - get_size_emu_sector();
		
	addr_flash = FLASH_BASE + size_flash;
	addr_flash -= sizeof(STR_ELEMENT);
	idx = 0;
	while((addr_flash >= addr_start) && (idx<SUM_ELEMENT))
	{
		P_STR_ELEMENT p_ele;
		p_ele = (P_STR_ELEMENT)addr_flash;
		if(p_ele->addr != 0xFFFF)
		{			
			if(-1 == find_ele_in_ram(p_ele->addr))
			{
				memcpy(&PARE[idx], (void*)addr_flash, sizeof(STR_ELEMENT));
				idx++;	
			}
		}
		else
		{
			addr_null = addr_flash;
		}
		addr_flash = addr_flash - sizeof(STR_ELEMENT);		
	}	
}
#else
void e2prom_init()
{
	uint32_t sec;
	uint32_t  addr_flash;
	uint32_t size_sec_flash;
	uint32_t idx;
	
	memset(PARE, 0xFF, sizeof(PARE));	//清空FF参数表
	
	sec = getSectorIndex(BASS_ADDR_EMU_FLASH);
	//sec = FLASH_If_GetSectorNumber(BASS_ADDR_EMU_FLASH);
	size_sec_flash = getSectorSize(sec) * 1024;
	addr_flash = BASS_ADDR_EMU_FLASH + size_sec_flash;
	addr_null = addr_flash;	//默认FLASH满了，需要擦除
	
	addr_flash -= sizeof(STR_ELEMENT);
	idx = 0;
	while((addr_flash >= BASS_ADDR_EMU_FLASH) && (idx<SUM_ELEMENT))
	{
		P_STR_ELEMENT p_ele;
		p_ele = (P_STR_ELEMENT)addr_flash;
		if(p_ele->addr != 0xFFFF)
		{			
			if(-1 == find_ele_in_ram(p_ele->addr))
			{
				memcpy(&PARE[idx], (void*)addr_flash, sizeof(STR_ELEMENT));
				idx++;	
			}
		}
		else
		{
			addr_null = addr_flash;
		}
		addr_flash = addr_flash - sizeof(STR_ELEMENT);		
	}	
}
#endif

int e2prom_wr_ele(P_STR_ELEMENT pdat)
{
	int idx;
	idx = find_ele_in_ram(pdat->addr);
	if(-1 != idx)
	{
		if(0 == memcmp(&PARE[idx], pdat, sizeof(STR_ELEMENT)))	//要写入的参数与FLASH保存的参数一样，无需再写
		{
			return 1;			
		}
		else
		{		
			memcpy(&PARE[idx], pdat, sizeof(STR_ELEMENT));
		}			
	}
	else{	//插入新元素
		wr_ele_in_ram(pdat);
	}
	//下面开始写入FLASH
	LL_wr_flash(pdat);
	return 1;
}

/*
*/
int e2prom_rd_ele(P_STR_ELEMENT pdat)
{
	int idx;
	idx = find_ele_in_ram(pdat->addr);
	if(-1 != idx)
	{
		pdat->addr = PARE[idx].addr;
		memcpy(pdat->dat, PARE[idx].dat, sizeof(STR_ELEMENT) -2);	
		return 1;
	}
	return -1;	
}

/*
在本机RAM缓冲中，寻找是否有匹配的变更，根据"地址标示"寻找
*/
int find_ele_in_ram(uint16_t addr)
{
	int i;
	for(i=0; i<SUM_ELEMENT; i++)
	{
		if(PARE[i].addr == addr)
		{
			return i;
		}
	}
	return -1;
}

/*
插入元素, 在PARE[i]找到第1个空位置，把参数拷贝进去
成功返回 1
失败返回 -1
*/
int wr_ele_in_ram(P_STR_ELEMENT pdat)
{
	int i;
	for(i=0; i<SUM_ELEMENT; i++)
	{
		if(PARE[i].addr == 0xFFFF)
		{
			memcpy(&PARE[i], pdat, sizeof(STR_ELEMENT));
			return 1;
		}
	}
	return -1;
}

/*
根据地址获得所在扇区号
*/
#if 1
uint32_t GetSectorNumber(uint32_t Address)	//willow change  20180118
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1))
  {
    sector = FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1))
  {
    sector = FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2))
  {
    sector = FLASH_SECTOR_6;  
  }
  else /*if((Address < USER_FLASH_END_ADDRESS) && (Address >= ADDR_FLASH_SECTOR_7_BANK2))*/
  {
    sector = FLASH_SECTOR_7;  
  }

  return sector;
}
#else
uint32_t GetSectorNumber(uint32_t Address)	//willow change  20180118
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;  
  }
  else
  {
    sector = FLASH_SECTOR_11;  
  }
  /*....如果大于12个扇区，可以在此添加*/
  return sector;
}
#endif

/*低级写*/
#if 1
int LL_wr_flash(P_STR_ELEMENT pdat)
{
	int i;
	uint32_t last_addr;
	last_addr = FLASH_BASE + ((*(uint16_t*)FLASHSIZE_BASE) << 10);
	HAL_FLASH_Unlock();	//FLASH_If_FlashUnlock();	
	if(addr_null == last_addr)	//FLASH已经写满，需要擦除
	{
		FLASH_EraseInitTypeDef FLASH_EraseInitStruct;
		uint32_t SectorError = 0;
		
		FLASH_EraseOneSectors(addr_start);	//最危险的操作，如果此时断电，那所有的参数都将丢失
//		FLASH_EraseInitStruct.TypeErase = TYPEERASE_SECTORS;	//FLASH_TYPEERASE_SECTORS;
//		FLASH_EraseInitStruct.Sector = GetSectorNumber(addr_start);
//		FLASH_EraseInitStruct.NbSectors = 1;
//		FLASH_EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;	//FLASH_VOLTAGE_RANGE_3;
//		if(HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &SectorError) != HAL_OK)
//			return (-1);
	
		addr_null = addr_start;
		for(i=0; i<SUM_ELEMENT; i++)
		{
			if(0xFFFF != PARE[i].addr)
			{
				FLASH_write(addr_null, &PARE[i] , sizeof(STR_ELEMENT));
				addr_null += sizeof(STR_ELEMENT);
			}			
		}	
	}
	else
	{		
		FLASH_write(addr_null, (void *) pdat , sizeof(STR_ELEMENT));
		addr_null += sizeof(STR_ELEMENT);		
	}
	HAL_FLASH_Lock();
	return 1;
}
#else
int LL_wr_flash(P_STR_ELEMENT pdat)
{
	int i;
	uint32_t last_addr;
	//last_addr = BASS_ADDR_EMU_FLASH + getSectorSize(getSectorIndex(BASS_ADDR_EMU_FLASH)) * 1024;
	last_addr = FLASH_BASE + ((*(uint16_t*)FLASHSIZE_BASE) << 10);
	if(addr_null == last_addr)	//FLASH使用完，需要擦除
	{
		HAL_FLASH_Unlock();
		FLASH_EraseOneSectors(addr_start);	//最危险的操作，如果此时断电，那所有的参数都将丢失
		addr_null = addr_start;
		for(i=0; i<SUM_ELEMENT; i++)
		{
			if(0xFFFF != PARE[i].addr)
			{
				FLASH_write(addr_null, &PARE[i] , sizeof(STR_ELEMENT));
				addr_null += sizeof(STR_ELEMENT);
			}			
		}
		HAL_FLASH_Lock();		
	}
	else
	{
		HAL_FLASH_Unlock();	//FLASH_If_FlashUnlock();
		FLASH_write(addr_null, (void *) pdat , sizeof(STR_ELEMENT));
		addr_null += sizeof(STR_ELEMENT);
		HAL_FLASH_Lock();
	}
	return 1;
}
#endif

/*
获得模拟e2prom的FLASH扇区大小， 默认为最后一个扇区的大小。 
*/
#if 0
/*自动获得模拟e2prom扇区大小*/
uint32_t get_size_emu_sector()
{
	uint32_t size_flash;
	uint32_t size_sector;
		
	size_flash = (*(uint16_t*)FLASHSIZE_BASE) << 10;
	
	if(size_flash == (ADDR_FLASH_SECTOR_0 - FLASH_BASE))
	{
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_1 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_1 - ADDR_FLASH_SECTOR_0;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_2 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_2 - ADDR_FLASH_SECTOR_1;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_3 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_3 - ADDR_FLASH_SECTOR_2;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_4 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_4 - ADDR_FLASH_SECTOR_3;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_5 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_5 - ADDR_FLASH_SECTOR_4;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_6 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_6 - ADDR_FLASH_SECTOR_5;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_7 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_7 - ADDR_FLASH_SECTOR_6;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_8 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_8 - ADDR_FLASH_SECTOR_7;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_9 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_9 - ADDR_FLASH_SECTOR_8;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_10 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_10 - ADDR_FLASH_SECTOR_9;
	}
	else if(size_flash == (ADDR_FLASH_SECTOR_11 - FLASH_BASE))
	{
		size_sector = ADDR_FLASH_SECTOR_11 - ADDR_FLASH_SECTOR_10;
	}
	/*....如果大于12个扇区，可以在此添加*/
	else{
		size_sector = FLASH_BASE + size_flash - ADDR_FLASH_SECTOR_11;
	}
	return size_sector;
}
#else
/*
手动设置模拟扇区大小
*/
uint32_t get_size_emu_sector()
{
	uint32_t  size;
#if defined 	STM32F105xx
	 size =  (2<<10);
#elif defined STM32F207xx
	 size =  (128<<10);
#elif defined STM32F437xx
	 size =  (128<<10);
#elif defined STM32H743xx
	 size =  (128<<10);
#endif
	return size;
}
#endif




