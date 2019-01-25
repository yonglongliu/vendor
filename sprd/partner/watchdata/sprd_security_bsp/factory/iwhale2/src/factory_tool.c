#include "sec_efuse_api.h"
#include "sec_efuse.h"
#include "dualOS_data.h"
#include "factory_tool.h"

extern uint32_t gPubKeyAddr;
//{85 62 BC 1A 0C F5 30 10 17 BD 3F 09 BF C0 39 1E 1F 5B 31 19 E0 EF 6D 5A 0B 94 66 62 23 3A A4 6B }
uint32_t hbk_buff[8] = {0x8562BC1A, 0x0CF53010, 0x17BD3F09, 0xBFC0391E, 0x1F5B3119, 0xE0EF6D5A, 0x0B946662, 0x233AA46B};
/*
 *sprd rotpk0 program
 */
int ft_sprd_rotpk0_program(void)
{
    int ret = 0;
    sprd_tee_debug("ft_sprd_rotpk0_program", "befor map share data phy addr %x", gPubKeyAddr);
    sprd_mmu_map_dev(gPubKeyAddr,4*1024,&pDualDateVal);

    sprd_tee_debug("ft_sprd_rotpk0_program", "after map share data virt addr %x", pDualDateVal);
    //ret = sprd_rotpk0_program(pDualDateVal->g_DX_Hbk);
    sprd_mmu_unmap_dev(pDualDateVal,4*1024);
    return ret;
}

/*
 *sprd get life cycle
 * */
int ft_sprd_get_lcs(unsigned int *pLcs)
{
    return sprd_get_lcs(pLcs);
}

/*
 * sprd write simlock
 * */
int ft_sprd_simlock_write(unsigned int block_offset, unsigned int writeData)
{
    //int ret;
    sprd_tee_debug("ft_sprd_simlock_write", "block_offset: %d, writeData: %d\n", block_offset, writeData);
    //ret = sprd_sec_efuse_simlock_write(block_offset, writeData);
    //return ret;
    return 0;
}

void ft_sprd_rotpk0_print(void)
{
    int i;
    uint32_t read_buffer[8] = {0};

    sprd_sec_efuse_rotpk0_read(read_buffer);
    sprd_tee_debug("ft_sprd_rotpk0_print", "***********read rotpk0 data start***************\n");

    for (i = 0; i < 8; i++)
        sprd_tee_debug("ft_sprd_rotpk0_print", "[%d]0x%x\n", i, read_buffer[i]);
    sprd_tee_debug("ft_sprd_rotpk0_print", "***********read rotpk0 data end***************\n");
}

/*
 *sprd rotpk1 program
 */
int ft_sprd_rotpk1_program(void)
{
	int ret = 0;
	//reserve for rotpk1
	//ret = sprd_rotpk1_program(pDualDateVal->g_DX_Hbk1);
	return ret;
}

/*
 *sprd enable rotpk1 bit
 */
int ft_sprd_rotpk1_enable(void)
{
	int ret = 0;
	ret = sprd_set_rotpk1_enable();
	return ret;
}

/*
 *sprd kce program
 */
int ft_sprd_kce_program(unsigned int* pKceData)
{
	int ret = 0;
	ret = sprd_kce_program(pKceData);
	return ret;
}



