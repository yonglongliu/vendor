/******************************************************************************
 ** File Name:    vsp_drv_sc8830.c
 *
 ** Author:       Xiaowei Luo                                                  *
 ** DATE:         06/20/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/20/2013    Xiaowei Luo      Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vpp_drv_interface.h"
#include "osal_log.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC int32 VSP_CFG_FREQ(VSPObject *vo, uint32 frame_size)
{
    if(frame_size <= 320*240)
    {
        vo->vsp_freq_div = 0;
    }
    else if(frame_size <= 720*576)
    {
        vo->vsp_freq_div = 1;
    }
    else if(frame_size < 1280 *720)
    {
        vo->vsp_freq_div = 2;
    }
    else
    {
        vo->vsp_freq_div = 3;
    }

    vo->vsp_freq_div += SPRD_VSP_FREQ_BASE_LEVEL;

    //using the highest freq. for vsp
    if (SPRD_VSP_FREQ_BASE_LEVEL){
        vo->vsp_freq_div = 4;
    }else{
        vo->vsp_freq_div = 3;
    }

    //SPRD_CODEC_LOGD ("%s, %d, vsp clock level: %d \n", __FUNCTION__, __LINE__,  vo->vsp_freq_div);

    return 0;
}

PUBLIC int32 VSP_OPEN_Dev (VSPObject *vo)
{
    int32 ret =0;
    char vsp_version_array[MAX_VERSIONS][10] = {"SHARK", "DOLPHIN", "TSHARK",
        "SHARKL", "PIKE", "PIKEL", "SHARKL64", "SHARKLT8", "WHALE","WHALE2","IWHALE2","ISHARKL2","SHARKL2"};

    if (-1 == vo->s_vsp_fd)
    {
        if((vo->s_vsp_fd = open(SPRD_VSP_DRIVER,O_RDWR)) < 0)
        {
            SPRD_CODEC_LOGE("open SPRD_VSP_DRIVER ERR\n");
            return -1;
        } else
        {
            vo->s_vsp_Vaddr_base = (uint_32or64)mmap(NULL,SPRD_VSP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED, vo->s_vsp_fd,0);
            vo->s_vsp_Vaddr_base -= VSP_REG_BASE_ADDR;
        }

        ioctl(vo->s_vsp_fd, VSP_VERSION, &(vo->vsp_version));
	ioctl(vo->s_vsp_fd, VSP_SET_CODEC_ID, &(vo->codec_id));
    }

    SPRD_CODEC_LOGD("%s, %d, vsp addr %lx, vsp_version: %s\n",__FUNCTION__, __LINE__, vo->s_vsp_Vaddr_base, vsp_version_array[vo->vsp_version]);

    return 0;
}

PUBLIC int32 VSP_CLOSE_Dev(VSPObject *vo)
{
    if(vo->s_vsp_fd > 0)
    {
        if (munmap((void *)(vo->s_vsp_Vaddr_base + VSP_REG_BASE_ADDR), SPRD_VSP_MAP_SIZE))
        {
            SPRD_CODEC_LOGE("%s, %d, %d", __FUNCTION__, __LINE__, errno);
            return -1;
        }

        close(vo->s_vsp_fd);
        return 0;
    } else
    {
        SPRD_CODEC_LOGD ("%s, error\n", __FUNCTION__);
        return -1;
    }
}

PUBLIC int32 VSP_GET_DEV_FREQ(VSPObject *vo, uint32*  vsp_clk_ptr)
{
    if(vo->s_vsp_fd > 0)
    {
        ioctl(vo->s_vsp_fd, VSP_GET_FREQ, vsp_clk_ptr);
        return 0;
    } else
    {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }
}

PUBLIC int32 VSP_CONFIG_DEV_FREQ(VSPObject *vo, uint32*  vsp_clk_ptr)
{
    if(vo->s_vsp_fd > 0)
    {
        int32 ret = ioctl(vo->s_vsp_fd, VSP_CONFIG_FREQ, vsp_clk_ptr);
        if (ret < 0)
        {
            SPRD_CODEC_LOGE ("%s, VSP_CONFIG_FREQ failed", __FUNCTION__);
            return -1;
        }
    } else
    {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }

    return 0;
}

PUBLIC int32 VSP_SET_SCENE_MODE(VSPObject *vo, uint32*  scene_mode)
{
    if(vo->s_vsp_fd > 0)
    {
        int32 ret;

        if(*scene_mode > MAX_SCENE_MODE)
        {
            SPRD_CODEC_LOGE ("%s, Invalid scene_mode value", __FUNCTION__);
            return -1;
        }

        ret = ioctl(vo->s_vsp_fd, VSP_SET_SCENE, scene_mode);
        if (ret < 0)
        {
            SPRD_CODEC_LOGE ("%s, VSP_SET_SCENE_MODE failed", __FUNCTION__);
            return -1;
        }
    } else
    {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }

    return 0;
}

PUBLIC int32 VSP_GET_SCENE_MODE(VSPObject *vo, uint32*  scene_mode)
{
    if(vo->s_vsp_fd > 0)
    {
        int32 ret = ioctl(vo->s_vsp_fd, VSP_GET_SCENE, scene_mode);
        if (ret < 0 || *scene_mode > MAX_SCENE_MODE)
        {
            SPRD_CODEC_LOGE ("%s, VSP_GET_SCENE_MODE failed", __FUNCTION__);
            return -1;
        }
    } else
    {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }

    return 0;
}

#define MAX_POLL_CNT    10
PUBLIC int32 VSP_POLL_COMPLETE(VSPObject *vo)
{
    if(vo->s_vsp_fd > 0)
    {
        int32 ret;
        int32 cnt = 0;

        do
        {
            ioctl(vo->s_vsp_fd, VSP_COMPLETE, &ret);
            cnt++;
        } while ((ret & V_BIT_30) &&  (cnt < MAX_POLL_CNT));
        if(!(V_BIT_1 == ret || V_BIT_2 == ret ))
        {
            SPRD_CODEC_LOGD("%s, %d, int_ret: %0x\n", __FUNCTION__, __LINE__, ret);
        }

        return ret;
    } else
    {
        SPRD_CODEC_LOGE ("%s, error", __FUNCTION__);
        return -1;
    }
}

PUBLIC int32 VSP_ACQUIRE_Dev(VSPObject *vo)
{
    int32 ret ;
    if(vo->s_vsp_fd <  0)
    {
        SPRD_CODEC_LOGE("%s: failed :fd <  0", __FUNCTION__);
        return -1;
    }

    ret = ioctl(vo->s_vsp_fd, VSP_ACQUAIRE, NULL);
    if(ret)
    {
#if 0
        SPRD_CODEC_LOGW ("%s: VSP_ACQUAIRE failed,  try again %d\n",__FUNCTION__, ret);
        ret =  ioctl(vo->s_vsp_fd, VSP_ACQUAIRE, NULL);
        if(ret < 0)
        {
            SPRD_CODEC_LOGE ("%s: VSP_ACQUAIRE failed, give up %d\n",__FUNCTION__, ret);
            return -1;
        }
#else
        SPRD_CODEC_LOGE("%s: VSP_ACQUAIRE failed,  %d\n",__FUNCTION__, ret);
        return -1;
#endif
    }

    if (VSP_CONFIG_DEV_FREQ(vo,&(vo->vsp_freq_div)))
    {
        return -1;
    }

    ret = ioctl(vo->s_vsp_fd, VSP_ENABLE, NULL);
    if (ret < 0)
    {
        SPRD_CODEC_LOGE("%s: VSP_ENABLE failed %d\n",__FUNCTION__, ret);
        return -1;
    }

    ret = ioctl(vo->s_vsp_fd, VSP_RESET, NULL);
    if (ret < 0)
    {
        SPRD_CODEC_LOGE("%s: VSP_RESET failed %d\n",__FUNCTION__, ret);
        return -1;
    }

    SPRD_CODEC_LOGD("%s, %d\n", __FUNCTION__, __LINE__);

    return 0;
}

PUBLIC int32 VSP_RELEASE_Dev(VSPObject *vo)
{
    int32 ret = 0;

    if(vo->s_vsp_fd > 0)
    {
        if (vo->error_flag)
        {
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_PAUSE_OFF, 0x1, "AXIM_PAUSE: stop DATA TRANS to AXIM");
            usleep(1);
        }

        if (VSP_READ_REG_POLL(GLB_REG_BASE_ADDR + AXIM_STS_OFF, V_BIT_1, 0x0, TIME_OUT_CLK_FRAME, "Polling AXIM_STS: not Axim_wch_busy")) //check all data has written to DDR
        {
            SPRD_CODEC_LOGE("%s, %d, Axim_wch_busy\n", __FUNCTION__, __LINE__);
        }

        if (VSP_READ_REG_POLL(GLB_REG_BASE_ADDR + AXIM_STS_OFF, V_BIT_2, 0x0, TIME_OUT_CLK_FRAME, "Polling AXIM_STS: not Axim_rch_busy"))
        {
            SPRD_CODEC_LOGE("%s, %d, Axim_wch_busy", __FUNCTION__, __LINE__);
        }

        ret = ioctl(vo->s_vsp_fd, VSP_RESET, NULL);
        if (ret < 0)
        {
            SPRD_CODEC_LOGE("%s: VSP_RESET failed %d\n",__FUNCTION__, ret);
            ret = -1;
            goto RELEASE_END;
        }

        ret = ioctl(vo->s_vsp_fd, VSP_DISABLE, NULL);
        if(ret < 0)
        {
            SPRD_CODEC_LOGE("%s: VSP_DISABLE failed, %d\n",__FUNCTION__, ret);
            ret = -1;
            goto RELEASE_END;
        }

        ret = ioctl(vo->s_vsp_fd, VSP_RELEASE, NULL);
        if(ret < 0)
        {
            SPRD_CODEC_LOGE("%s: VSP_RELEASE failed, %d\n",__FUNCTION__, ret);
            ret = -1;
            goto RELEASE_END;
        }
    } else
    {
        SPRD_CODEC_LOGE("%s: failed :fd <  0", __FUNCTION__);
        ret = -1;
    }

RELEASE_END:

    if (vo->error_flag || (ret < 0))
    {
        usleep(20*1000);
    }

    SPRD_CODEC_LOGD("%s, %d, ret: %d\n", __FUNCTION__, __LINE__, ret);

    return ret;
}

int32 VSP_Get_Codec_Counter(VSPObject *vo, uint32* counter)
{
    int ret = 0;
    ret = ioctl(vo->s_vsp_fd,VSP_GET_CODEC_COUNTER,counter);
    return ret;
}
PUBLIC int VSP_Get_IOVA(VSPObject *vo, int fd, unsigned long *iova, size_t *size)
{
    int ret = 0;
    VSP_IOMMU_MAP_DATA_T map_data;

    map_data.fd = fd;
    ret = ioctl(vo->s_vsp_fd,VSP_GET_IOVA,&map_data);

    if(ret < 0 )
    {
        SPRD_CODEC_LOGE("vsp_get_iova failed ret %d \n",ret);
    }
    else
    {
        *iova = map_data.iova;
        *size = map_data.size;
    }

    return ret;
}

PUBLIC int VSP_Free_IOVA(VSPObject *vo,  unsigned long iova, size_t size)
{
    int ret = 0;
    VSP_IOMMU_MAP_DATA_T unmap_data;

    if(iova == 0) {
        return -1;
    }
    unmap_data.iova = iova;
    unmap_data.size = size;

    ret = ioctl(vo->s_vsp_fd,VSP_FREE_IOVA, &unmap_data);

    if(ret < 0 )
    {
        SPRD_CODEC_LOGE("vsp_free_iova failed ret %d \n",ret);
    }

   return ret;
}

PUBLIC int VSP_Get_IOMMU_Status(VSPObject *vo)
{
    int ret = 0;

    ret = ioctl(vo->s_vsp_fd,VSP_GET_IOMMU_STATUS);

    return ret;
}
PUBLIC int VSP_NeedAlign(VSPObject *vo)
{
    if((vo->vsp_version == WHALE2) || (vo->vsp_version == IWHALE2))
    {
        return 0;
    }
    return 1;

}

PUBLIC int32 ARM_VSP_RST (VSPObject *vo)
{
    if(VSP_ACQUIRE_Dev(vo) < 0)
    {
        return -1;
    }

    VSP_WRITE_REG(AHB_CTRL_BASE_ADDR + ARM_ACCESS_CTRL_OFF, 0,"RAM_ACC_by arm");
    if (VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR + ARM_ACCESS_STATUS_OFF, (V_BIT_1 | V_BIT_0), 0x00000000, TIME_OUT_CLK, "ARM_ACCESS_STATUS_OFF"))
    {
        return -1;
    }

    VSP_WRITE_REG(AHB_CTRL_BASE_ADDR + ARM_INT_MASK_OFF, 0,"arm INT mask set");	// Disable Openrisc interrrupt . TBD.
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF, 0, "VSP_INT_MASK, enable vlc_slice_done, time_out");

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, 0x30828,"axim endian set"); // VSP and OR endian.
    return 0;
}

PUBLIC  int32 vsp_read_reg_poll(VSPObject *vo, uint32 reg_addr, uint32 msk_data,uint32 exp_value, uint32 time, char *pstring)
{
    uint32 tmp, vsp_time_out_cnt = 0;

    tmp=(*((volatile uint32*)(reg_addr+((VSPObject *)vo)->s_vsp_Vaddr_base)))&msk_data;
    while((tmp != exp_value) && (vsp_time_out_cnt < time))
    {
        tmp=(*((volatile uint32*)(reg_addr+((VSPObject *)vo)->s_vsp_Vaddr_base)))&msk_data;
        vsp_time_out_cnt++;
    }

    if (vsp_time_out_cnt >= time)
    {
        uint32 mm_eb_reg;

        ioctl(vo->s_vsp_fd, VSP_HW_INFO, &mm_eb_reg);
        vo->error_flag |= ER_HW_ID;
        SPRD_CODEC_LOGE ("vsp_time_out_cnt %d, MM_CLK_REG (0x402e0000): 0x%0x",vsp_time_out_cnt, mm_eb_reg);
        return 1;
    }

    return 0;
}

// Call DMA to transfer table
PUBLIC void VSP_DMA_TRANSFER(VSPObject *vo, uint_32or64 src_addr,  uint32 dst_addr, uint32 size)
{
#if defined CHIP_WHALE
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF, V_BIT_8, "VSP_INT_MASK");
#ifdef CHIP_IWHALE2
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF, 0, "VSP_INT_MASK");
#endif
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+DMA_SET0_OFF, 0, "DMA trans dir 0, from EXT to IRAM");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+DMA_SET1_OFF, size, "DMA trans size");   // unit=128-bit, 72 words
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+DMA_SET2_OFF, src_addr, "DMA EXT start addr");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+DMA_SET3_OFF, dst_addr, "DMA VLC table start addr");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, V_BIT_8|V_BIT_6, "DMA_access_mode=1&& watchdog_disable");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_START_OFF, 0x1, "DMA_START");
    VSP_READ_REG_POLL(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, V_BIT_8, V_BIT_8, TIME_OUT_CLK, "DMA_TRANS_DONE int");//check HW int
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, V_BIT_8, "clear VSP_INT_RAW");
    VSP_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_CLR_OFF, 0x7, "clear ARM_INT_RAW");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF, 0x0, "VSP_INT_MASK");
#endif
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
