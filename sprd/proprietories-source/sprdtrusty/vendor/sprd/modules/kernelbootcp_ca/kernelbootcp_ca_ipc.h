/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

__BEGIN_DECLS

/* Commands for communicating with secureboot_ta */
//#define KERNEL_BOOTCP_VERIFY_ALL  1
//#define KERNEL_BOOTCP_UNLOCK_DDR  2
/**--------------------------------------------------------------------------*
 **                         TYPE AND CONSTANT                                *
 **--------------------------------------------------------------------------*/
 typedef struct{
    uint32_t  mMagicNum;        // "BTHD"=="0x42544844"=="boothead"
    uint32_t  mVersion;         // 1
    uint8_t   mPayloadHash[32]; // sha256 hash value
    uint64_t  mImgAddr;         // image loaded address
    uint32_t  mImgSize;         // image size
    uint32_t  is_packed;        // packed image flag 0:false 1:true
    uint32_t  mFirmwareSize;    // runtime firmware size
    uint8_t   reserved[452];    // 452 + 15*4 = 512
}sys_img_header;

typedef struct{
    uint64_t img_addr;     // the base address of image to verify
    uint32_t img_len;      // length of image
    uint8_t  pubkhash[32]; // pubkey hash for verifying image
    uint32_t flag;         // sprd or sanda plan
}kbcImgInfo;

typedef struct {
    uint64_t img_addr;  // the base address of image to verify
    uint32_t img_len;   // length of image
    uint32_t map_len;   // mapping length
} KBC_IMAGE_S;

typedef struct {
  KBC_IMAGE_S modem;
  KBC_IMAGE_S ldsp;
  KBC_IMAGE_S tgdsp;
  KBC_IMAGE_S pm_sys;
  uint16_t    flag;      // sprd or sanda plan
  uint16_t    is_packed; // is packed image
  uint8_t     cntcert[1024];
} KBC_LOAD_TABLE_S;

int trusty_kernelbootcp_connect(void);
int trusty_kernelbootcp_call(uint32_t cmd, void *in, uint32_t in_size, uint8_t *out,
         uint32_t *out_size);
void trusty_kernelbootcp_disconnect(void);
int kernel_bootcp_unlock_ddr(void);
int kernel_bootcp_verify_all(KBC_LOAD_TABLE_S  *table);

__END_DECLS
