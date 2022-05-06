/* bps_emu_e2prom.c
ʹ���ڲ�FLASH�����һ��������ģ��E2PROM�����澭���ı�Ĳ�����
�˲����Բ����ṹ�����ʽ������flash��
struct element{
	uint16_t addr;
	uint8_t dat[30];
};
����addrΪ������ַ��ʾ����������ͬ�Ĳ�����
dat[30]��Ϊ��Ч�����ռ䣬�����Ա���30B�������Ҫ��չ�����԰Ѵ˽ṹ�����չ��32B�����������ɣ�Ϊ�˼���STM32H7ϵͳ�ڲ�FLASH��
ʹ��˵����
1. ��BASS_ADDR_EMU_FLASH�׵�ַ������(��������ʹ�����һ��������Ϊģ��FLASH)
2. �ϵ���ȳ�ʼ����e2prom_init()
3. ��ʼ����Ϳ��Զ�дҪ����Ĳ����ˡ�
4. ���������ֻ��1�������� 128k/32*10000=4000W��,��������������

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
//#define		SUM_ELEMENT			(10U)	//���ģ���Ԫ�ظ���

#ifndef STM32H743xx
#define FLASHSIZE_BASE               0x1FFF7A22UL           /*!< FLASH Size register base address       */
#endif

STR_ELEMENT PARE[SUM_ELEMENT];
//uint8_t * p_addr;
uint32_t addr_null;		//����д���һ���մ洢��Ԫ��ַ
uint32_t addr_start;	//ģ��E2PROM�Ŀ�ʼ��ַ

int LL_wr_flash(P_STR_ELEMENT pdat);
int find_ele_in_ram(uint16_t addr);
int wr_ele_in_ram(P_STR_ELEMENT pdat);
uint32_t get_size_emu_sector(void);


/*
��ʼ��E2PROM
*/
#if 1
void e2prom_init()
{
	uint32_t  addr_flash;
	uint32_t idx;
	uint32_t size_flash;
	
	memset(PARE, 0xFF, sizeof(PARE));	//���FF������
	
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
	
	memset(PARE, 0xFF, sizeof(PARE));	//���FF������
	
	sec = getSectorIndex(BASS_ADDR_EMU_FLASH);
	//sec = FLASH_If_GetSectorNumber(BASS_ADDR_EMU_FLASH);
	size_sec_flash = getSectorSize(sec) * 1024;
	addr_flash = BASS_ADDR_EMU_FLASH + size_sec_flash;
	addr_null = addr_flash;	//Ĭ��FLASH���ˣ���Ҫ����
	
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
		if(0 == memcmp(&PARE[idx], pdat, sizeof(STR_ELEMENT)))	//Ҫд��Ĳ�����FLASH����Ĳ���һ����������д
		{
			return 1;			
		}
		else
		{		
			memcpy(&PARE[idx], pdat, sizeof(STR_ELEMENT));
		}			
	}
	else{	//������Ԫ��
		wr_ele_in_ram(pdat);
	}
	//���濪ʼд��FLASH
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
�ڱ���RAM�����У�Ѱ���Ƿ���ƥ��ı��������"��ַ��ʾ"Ѱ��
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
����Ԫ��, ��PARE[i]�ҵ���1����λ�ã��Ѳ���������ȥ
�ɹ����� 1
ʧ�ܷ��� -1
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
���ݵ�ַ�������������
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
  /*....�������12�������������ڴ����*/
  return sector;
}
#endif

/*�ͼ�д*/
#if 1
int LL_wr_flash(P_STR_ELEMENT pdat)
{
	int i;
	uint32_t last_addr;
	last_addr = FLASH_BASE + ((*(uint16_t*)FLASHSIZE_BASE) << 10);
	HAL_FLASH_Unlock();	//FLASH_If_FlashUnlock();	
	if(addr_null == last_addr)	//FLASH�Ѿ�д������Ҫ����
	{
		FLASH_EraseInitTypeDef FLASH_EraseInitStruct;
		uint32_t SectorError = 0;
		
		FLASH_EraseOneSectors(addr_start);	//��Σ�յĲ����������ʱ�ϵ磬�����еĲ���������ʧ
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
	if(addr_null == last_addr)	//FLASHʹ���꣬��Ҫ����
	{
		HAL_FLASH_Unlock();
		FLASH_EraseOneSectors(addr_start);	//��Σ�յĲ����������ʱ�ϵ磬�����еĲ���������ʧ
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
���ģ��e2prom��FLASH������С�� Ĭ��Ϊ���һ�������Ĵ�С�� 
*/
#if 0
/*�Զ����ģ��e2prom������С*/
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
	/*....�������12�������������ڴ����*/
	else{
		size_sector = FLASH_BASE + size_flash - ADDR_FLASH_SECTOR_11;
	}
	return size_sector;
}
#else
/*
�ֶ�����ģ��������С
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




