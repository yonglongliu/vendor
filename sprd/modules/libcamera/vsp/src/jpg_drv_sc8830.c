/******************************************************************************
 ** File Name:      vsp_drv.c                                                 *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    JPG Driver												  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sci_types.h"
#include "jpg_drv_sc8830.h"
#include "jpegcodec_global.h"
#if !defined(_VSP_)
#include "common_global.h"
#include "bsm_global.h"
#endif //_CMODEL_

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 
#ifdef _VSP_LINUX_

unsigned long g_jpg_Vaddr_base = 0;
int g_jpg_dev_fd = 0;
FunctionType_ResetJPG ResetJPG_cb=NULL;
PUBLIC void  JPG_SetVirtualBaseAddr(unsigned long jpg_Vaddr_base)
{	
	g_jpg_Vaddr_base = jpg_Vaddr_base;
}

PUBLIC void  JPG_reg_reset_callback(FunctionType_ResetJPG cb,int fd)
{
	ResetJPG_cb = cb;
	g_jpg_dev_fd = fd;
}

#endif


#define VSP_96MHz	0x00
#define VSP_64MHz	0x01
#define VSP_48MHz	0x10
#define VSP_26MHz	0x11

/************************************************************************/
/* Reset HW                                                             */
/************************************************************************/
PUBLIC void  JPG_Reset(void)
{
#ifdef _VSP_LINUX_
	if(ResetJPG_cb)
		(*ResetJPG_cb)(g_jpg_dev_fd);
#else
	uint32 cmd = 0;
	
	cmd = JPG_READ_REG(JPG_CLK_EN_REG, "JPG_CLK: Read JPG clock");
	JPG_WRITE_REG(JPG_CLK_EN_REG, cmd|(1<<5), "JPG_CLK: enable JPG clock");

	cmd = JPG_READ_REG(AXI_CLK_EN_REG, "AXI_CLK: Read AXI clock");
	JPG_WRITE_REG(AXI_CLK_EN_REG, cmd|(1<<6), "AXI_CLK: enable AXI clock");
		
	/*reset vsp*/
	cmd = JPG_READ_REG(JPG_RESET_REG, "VSP_RESET_ADDR: Read the vsp reset");
	JPG_WRITE_REG(JPG_RESET_REG, cmd | (1<<6), "VSP_RESET_REG: only reset vsp, don't reset dcam");
	JPG_WRITE_REG(JPG_RESET_REG, cmd | (0<<6), "VSP_RESET_REG: only reset vsp, don't reset dcam");

//	cmd = VSP_READ_REG(0x8b000070, "");
//	cmd &= ~0xc;
//	cmd |= (VSP_64MHz<<2);
//	VSP_WRITE_REG(0x8b000070, cmd, "vsp: 64MHz");
#endif		
	//for little endian system
//#ifdef CHIP_ENDIAN_LITTLE
//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, 0x5, "ENDAIN_SEL: 0x5 for little endian system");
//#else
//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, 0x0, "ENDAIN_SEL: 0x5 for big endian system");
//#endif
}

/*only generate firmware command*/
PUBLIC void flush_unalign_bytes(int32 nbytes)
{
	int i = 0;
	uint32 cmd = 0;

	cmd = (8<<24) | 1;

	for (i = 0; i < nbytes; i++)
	{
//		if(VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 1<<3, 1<<3, TIME_OUT_CLK,
//			"polling bsm fifo fifo depth >= 8 words for gob header"))
//		{
//			return;
//		}

            JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_STS0_OFFSET, ((uint32)1<<31), ((uint32)0<<31), TIME_OUT_CLK, "BSM_DEBUG, polling bsm status");

		JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG2_OFFSET, cmd, "BSM_CFG2: flush one byte");
	}
}

//allow software to access the vsp buffer
PUBLIC void open_jpg_iram (void)
{
	uint32 cmd;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();

	cmd = JPG_READ_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, "DCAM_CFG: allow software to access the vsp buffer");
	cmd |= (1<<2);
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, cmd, "DCAM_CFG: allow software to access the vsp buffer");

	if ((jpeg_fw_codec->jpg_version != SHARKL2) && (jpeg_fw_codec->jpg_version != IWHALE2) && (jpeg_fw_codec->jpg_version != SHARKLE))
		JPG_READ_REG_POLL(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, 1<<8, 1<<8, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");
}

//allow hardware to access the vsp buffer
PUBLIC void close_jpg_iram (void)
{
	uint32 cmd;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();

	//	cmd = (0<<4) | (1<<3);
	cmd = JPG_READ_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, "DCAM_CFG: allow hardware to access the vsp buffer");
	cmd = (cmd & ~0x4) ;
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, cmd, "DCAM_CFG: allow hardware to access the vsp buffer");

	if ((jpeg_fw_codec->jpg_version != SHARKL2) && (jpeg_fw_codec->jpg_version != IWHALE2) && (jpeg_fw_codec->jpg_version != SHARKLE) )
		JPG_READ_REG_POLL (JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, 1,1, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");
}

/**
configure the huffman table
**/
PUBLIC void configure_huff_tab(uint32 *pHuff_tab, int32 n)
{
	int i = 0;
	uint32 cmd = 0;
	uint32 val = 0;
	
	open_jpg_iram();

	for(i = 0; i < n; i++)
	{
		val = *pHuff_tab++;
		
		JPG_WRITE_REG(HUFFMAN_TBL_ADDR+i*4, val, "HUFFMAN_TBL_ADDR: configure vlc table");
	}

	close_jpg_iram();
}

/**
stop vsp
**/
PUBLIC void Vsp_Stop()
{
	uint32 cmd = 0;
	
	/*reset dcam and vsp*/
//	cmd = VSP_READ_REG(VSP_RESET_ADDR, "VSP_RESET_ADDR: Read the vsp reset");
//	VSP_WRITE_REG(VSP_RESET_ADDR, cmd | (/*(1<<2) |*/ (1<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
//	VSP_WRITE_REG(VSP_RESET_ADDR, cmd | (/*(0<<2) |*/ (0<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
}


PUBLIC int JPG_Get_IOVA(int jpg_fd, int fd, unsigned long *iova, size_t *size)
{
    int ret = 0;
    struct jpg_iommu_map_data map_data;

    map_data.fd = fd;
    ret = ioctl(jpg_fd,JPG_GET_IOVA,&map_data);

    if(ret < 0 )
    {
        SCI_TRACE_LOW("JPG_get_iova failed ret %d \n",ret);
    }
    else
    {
        *iova = map_data.iova_addr;
        *size = map_data.size;
    }

    return ret;
}

PUBLIC int JPG_Free_IOVA(int jpg_fd,  unsigned long iova, size_t size)
{
    int ret = 0;
    struct jpg_iommu_map_data unmap_data;

    unmap_data.iova_addr = iova;
    unmap_data.size = size;

    ret = ioctl(jpg_fd,JPG_FREE_IOVA, &unmap_data);

    if(ret < 0 )
    {
        SCI_TRACE_LOW("jpg_free_iova failed ret %d \n",ret);
    }

   return ret;
}

PUBLIC int VSP_Get_IOMMU_Status(int jpg_fd)
{
    int ret = 0;

    ret = ioctl(jpg_fd,JPG_GET_IOMMU_STATUS);

    return ret;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
