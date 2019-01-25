/******************************************************************************
 ** File Name:    vsp_deint_api.c                                                  *
 ** Author: *
 ** DATE:         5/4/2017                                                   *
 ** Copyright:    2017 Spreadtrum, Incorporated. All Rights Reserved.         *
 ** Description:  define data structures for vsp deinterlace                      *
 *****************************************************************************/

#include "vpp_drv_interface.h"

#include "osal_log.h"
#define LOG_TAG "VSPDEINTAPI"

int init_deint_reg(VPPObject *vo, DEINT_PARAMS_T *p_deint_params,
                   uint32 frame_no) {
    uint32 val;

    val = V_BIT_23 | V_BIT_21 |V_BIT_20 | V_BIT_18;
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, val,"axim endian set"); //VSP and OR endian.
    //VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");
    val = V_BIT_30;
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, val,"enable deintelace");

#ifdef USE_INTERRUPT
    VSP_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
#endif
    if (0 == frame_no) {
        val   = (1 & 0x1) ;
    }else{
        val   = (0 & 0x1) ;
    }
    val |= (0 & 0x2) << 1;  //yuv format : 0: UV,       1:u/v
    val |= (p_deint_params->threshold & 0xff) << 8;
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG0_OFF, val, "deinterlace cfg 0");

    if ((p_deint_params->width >> 12) + (p_deint_params->height >> 12) > 0) {
        return -1;
    }
    val = p_deint_params->width;
    val |= (p_deint_params->height << 16);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG1_OFF, val, "deinterlace img size");

    return 0;
}

int write_deint_frame_addr(VPPObject *vo, unsigned long src_frame,
                           unsigned long ref_frame, unsigned long dst_frame,
                           DEINT_PARAMS_T *p_deint_params) {
    uint32 y_len = p_deint_params->y_len;
    uint32 c_len = p_deint_params->c_len;
    unsigned long src_frame_addr = src_frame;
    unsigned long ref_frame_addr = ref_frame;
    unsigned long dst_frame_addr = dst_frame;

    if (p_deint_params->interleave) {
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG2_OFF, (src_frame_addr) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF, (src_frame_addr + y_len) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG4_OFF, 0, "");

        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF, (dst_frame_addr) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG6_OFF, (dst_frame_addr + y_len) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG7_OFF, 0, "");

        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG8_OFF, (ref_frame_addr) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG9_OFF, (ref_frame_addr + y_len) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG10_OFF, 0, "");
    } else {
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG2_OFF, (src_frame_addr) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF, (src_frame_addr + y_len) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG4_OFF, (src_frame_addr + y_len + c_len) >> 3, "");

        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF, (dst_frame_addr) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG6_OFF, (dst_frame_addr + y_len) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG7_OFF, (dst_frame_addr + y_len + c_len) >> 3, "");

        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG8_OFF, (ref_frame_addr) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG9_OFF, (ref_frame_addr + y_len) >> 3, "");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG10_OFF, (ref_frame_addr + y_len + c_len) >> 3, "");
    }

    return 0;
}

int32 vpp_deint_init(VPPObject *vo) {
    int ret;

    if (NULL == vo) {
        return -1;
    }

    memset(vo, 0, sizeof(VPPObject));
    vo->s_vsp_fd = -1;
    vo->vsp_freq_div = 3;
    ///vo->trace_enabled = 1;
    ret = VSP_OPEN_Dev(vo);
    if (ret != 0) {
        SPRD_CODEC_LOGE("vsp_open_dev failed ret: 0x%x\n", ret);
        return -1;
    }

    return 0;
}

int vpp_deint_get_iova(VPPObject *vo, int fd, unsigned long *iova,
                       size_t *size) {
  int ret = 0;

  ret = VSP_Get_IOVA(vo, fd, iova, size);

  return ret;
}

int vpp_deint_free_iova(VPPObject *vo, unsigned long iova, size_t size) {
  int ret = 0;

  ret = VSP_Free_IOVA(vo, iova, size);

  return ret;
}

int vpp_deint_get_IOMMU_status(VPPObject *vo) {
  int ret = 0;

  ret = VSP_Get_IOMMU_Status(vo);

  return ret;
}

int32 vpp_deint_release(VPPObject *vo) {
  if (vo) {
    VSP_CLOSE_Dev(vo);
  }

  return 0;
}

int32 vpp_deint_process(VPPObject *vo, unsigned long src_frame,
                        unsigned long ref_frame, unsigned long dst_frame,
                        uint32 frame_no, DEINT_PARAMS_T *p_deint_params) {
    SPRD_CODEC_LOGI("vsp_deint_process: src_frame = 0x%lx, ref_frame = 0x%lx, dst_frame = "
                                "0x%lx, frame_no = %d\n", src_frame, ref_frame, dst_frame, frame_no);

    if(ARM_VSP_RST(vo)<0){
        return -1;
    }

    if (init_deint_reg(vo, p_deint_params, frame_no)) {
        SPRD_CODEC_LOGE("Init params error\n");
        VSP_RELEASE_Dev(vo);
        return -1;
    }

    SPRD_CODEC_LOGD("%s, %d, interleave : %d", __FUNCTION__,__LINE__, p_deint_params->interleave);
    write_deint_frame_addr(vo, src_frame, ref_frame, dst_frame, p_deint_params);

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0xfFF,"clear BSM_frame done int");
    ///VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 1,"RAM_ACC_SEL");//change ram access to vsp hw
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_DEINTELACE_OFF, 0x01, "deinterlace path start");

#ifdef USING_INTERRUPT
    if (VSP_POLL_COMPLETE(vo)) {
        SPRD_CODEC_LOGE("Error, Get interrupt timeout!!!! \n");
        VSP_RELEASE_Dev(vo);
        return -1;
    }
#else
    {
        uint32 cmd = VSP_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
        SPRD_CODEC_LOGD("%s, %d, cmd : %d", __FUNCTION__,__LINE__, cmd);
        while ((cmd & V_BIT_9) == 0){
            usleep(200);
            cmd = VSP_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
            SPRD_CODEC_LOGD("%s, %d, cmd : %d", __FUNCTION__,__LINE__, cmd);
        }
    }
#endif
    //VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0, "RAM_ACC_SEL");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1FF, "VSP_INT_CLR, clear all prossible interrupt");

    VSP_RELEASE_Dev(vo);
    return 0;
}

