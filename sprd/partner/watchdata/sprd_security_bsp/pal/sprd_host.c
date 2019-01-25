/*
*  Copyright 2016 (c) Spreadtrum Communications Inc.
*
*  This software is protected by copyright, international treaties and various patents.
*  Any copy, reproduction or otherwise use of this software must be authorized in a
*  license agreement and include this Copyright Notice and any other notices specified
*  in the license agreement. Any redistribution in binary form must be authorized in the
*  license agreement and include this Copyright Notice and any other notices specified
*  in the license agreement and/or in materials provided with the binary distribution.
*
*  created by vee.zhang <2016.10.15>
*/

//#include "mmu_api.h"
#include "sprd_host.h"
#include "sprd_ecc.h"
//#include "tee_internal_api.h"

extern uint32_t wd_mmu_map( uint32_t physicalAddress, uint32_t mapSize, uint32_t **ppVirtBuffAddr);
extern uint32_t wd_mmu_unmap(uint32_t *pVirtBuffAddr, uint32_t mapSize);

#undef SPRD_CE_DEBUG

#define  CE_8M_PADDR	(0x8000000 + 84*1024*1024)
#define  CE_REG_PADDR	(0xC0D00000)
#define  CE_CTL_PADDR	(0xE2210000)
#define  CE_MAX_SIZE    128

typedef struct  map_table{
	uint32_t paddr;
	uint32_t vaddr;
	uint32_t size;
	uint8_t is_used;
} map_table_t;

uint32_t *gSprdRegBase = 0x00;
uint32_t *gCtxBuffer = 0x00;
uint32_t *gCeCtlReg  = 0x00;

static map_table_t ce_map_table[CE_MAX_SIZE]={{0,0,0,0},};


/*
 * map pa to va
 * Return 0 in case of success
 * Return 1 in case of error
 */
uint32_t sprd_mmu_map( uint32_t physicalAddress, uint32_t mapSize, uint32_t **ppVirtBuffAddr)
{
	int i;

	for(i =0;i<CE_MAX_SIZE;i++) {
		if((ce_map_table[i].is_used == 1)&&
				(ce_map_table[i].paddr == (uint32_t)physicalAddress)&&
				(ce_map_table[i].size == mapSize)){
				CE_PF("sprd_mmu_map paddr already mapped !.");
				*ppVirtBuffAddr = (uint32_t *)ce_map_table[i].vaddr;

				return 0;
		}
	}

	for(i =0;i<CE_MAX_SIZE;i++){
		if(ce_map_table[i].is_used == 0)
			break;
	}

	if(i>=CE_MAX_SIZE){
		CE_PF("sprd_mmu_map no memory !.");
		return -1;
	}

	wd_mmu_map(physicalAddress , mapSize, ppVirtBuffAddr);
	CE_PF("physical %x size %d ppVirtBuffAddr : %p\n",physicalAddress, mapSize, *ppVirtBuffAddr);
	if (*ppVirtBuffAddr == 0) {
		CE_PF("sprd_mmu_map no memory !.");
		return -1;
	}
	ce_map_table[i].is_used = 1;
	ce_map_table[i].paddr = physicalAddress;
	ce_map_table[i].vaddr = (uint32_t)*ppVirtBuffAddr;
	ce_map_table[i].size  = mapSize;

	return 0;
}
/*
 * unmap va
 * Return 0 in case of success
 * Return 1 in case of error
 */
uint32_t sprd_mmu_unmap(uint32_t *pVirtBuffAddr, uint32_t mapSize)
{
	for(int i =0;i<CE_MAX_SIZE;i++){
		if((ce_map_table[i].is_used == 1)&&
			(ce_map_table[i].vaddr == (uint32_t)pVirtBuffAddr)&&
			(ce_map_table[i].size == mapSize)){

			wd_mmu_unmap(pVirtBuffAddr, mapSize);
			CE_PF("size %d ppVirtBuffAddr : %p\n",mapSize, pVirtBuffAddr);

			ce_map_table[i].is_used =0;
			return 0;
		}
	}

	return -1;
}

uint32_t sprd_va2pa_map(uint32_t vir_addr)
{
	int paddr = 0;
	for(int i =0;i<CE_MAX_SIZE;i++){
		if((ce_map_table[i].is_used == 1) &&
		   (vir_addr >= ce_map_table[i].vaddr)&&
		   (vir_addr <= (ce_map_table[i].vaddr + ce_map_table[i].size))) {
			paddr = (vir_addr - ce_map_table[i].vaddr + ce_map_table[i].paddr);
			//CE_PF("vir_addr = 0x%x  phy_addr = 0x%x\n",vir_addr, paddr);
			return paddr;
		}
	}

	return paddr;
}

int sprd_ce_init(void)
{
	uint32_t ret = 0;

	ret = sprd_mmu_map(CE_CTL_PADDR, 1*32, &gCeCtlReg);
	if (ret) {
		CE_PF("sprd_mmu_map CE_CTL_PADDR error, ret = %d.", ret);
		return -1;
	}

	ret = sprd_mmu_map(CE_REG_PADDR, 1*1024, &gSprdRegBase);
	if (ret) {
		sprd_mmu_unmap(gCeCtlReg, 1*32);
		CE_PF("sprd_mmu_map CE_REG_PADDR error, ret = %d.", ret);
		return -1;
	}

	ret |= sprd_mmu_map(CE_8M_PADDR, 8*1024*1024, &gCtxBuffer);
	if (ret) {
		sprd_mmu_unmap(gCeCtlReg, 1*32);
		sprd_mmu_unmap(gSprdRegBase, 1*1024);
		CE_PF("sprd_mmu_map CE_8M_PADDR error, ret = %d.", ret);
		return -1;
	}

	*gCeCtlReg |= BIT_AP_AHB_RF_CE0_EB;

	sprd_ecc_init();

	return 0;
}

int sprd_ce_terminal(void)
{
	sprd_mmu_unmap(gSprdRegBase, 1*1024);
	sprd_mmu_unmap(gCtxBuffer, 8*1024*1024);
	sprd_mmu_unmap(gCeCtlReg, 1*32);

	return 0;
}
/*****************************************************************************/
