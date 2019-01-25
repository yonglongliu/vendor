/*
 *    Copyright (C) 2013 SAMSUNG S.LSI
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *   Author: Woonki Lee <woonki84.lee@samsung.com>
 *   Version: 2.0
 *
 */

#ifndef __NFC_SEC_PRODUCT__
#define __NFC_SEC_PRODUCT__

/* products */
typedef enum {
    SNFC_CHECKING = 0,
    SNFC_UNKNOWN = 0x10,
    SNFC_N3 = 0x30,
    SNFC_N5 = 0x50,
    SNFC_N5_5_0_0_0,
    SNFC_N5_5_0_0_1,
    SNFC_N5_5_0_x_2,
    //SNFC_N5_5_0_x_4,
    SNFC_N5P = 0x60,
    SNFC_N5P_5_0_x_3,
    SNFC_N5P_5_0_x_5,
    SNFC_N5P_5_0_x_6,    // N5ABP
    SNFC_N5P_5_0_x_7,
    SNFC_N5S_5_0_x_8,   // N5S
    SNFC_N7 = 0x70,
    SNFC_N7_7_0_0_x,
    SNFC_N8 = 0x80,
    SNFC_N81,
	SNFC_N82
} eNFC_SNFC_PRODUCTS;

/* Products information */
static struct _sproduct_info {
    eNFC_SNFC_PRODUCTS group;
    uint8_t major_version;
    char *chip_name;
} _product_info[] = {
    {SNFC_N3,   0x10, "S3FRNR3"}
    ,{SNFC_N5,  0x10, "S3FWRN5"}
    ,{SNFC_N5P, 0x20, "S3FWRN5P"}
    ,{SNFC_N7,  0x30, "S3FWRN7"}
    ,{SNFC_N8,  0x30, "S3FWRN8"}
    ,{SNFC_N81, 0x40, "S3FWRN81"}
	,{SNFC_N82, 0x50, "S3FWRN82"}
    };

#define productGroup(x) (x & 0xF0)

static uint8_t _n3_bl_ver_list[][6] = {
        // 4 bytes of version, 1 bytes of product, 1 bytes of base address
        {0x00, 0x00, 0x00, 0x00, SNFC_UNKNOWN, 0x80}
        ,{0x05, 0x00, 0x03, 0x00, SNFC_N3, 0x80}
        };
static uint8_t _n5_bl_ver_list[][6] = {
        {0x00, 0x00, 0x00, 0x00, SNFC_UNKNOWN, 0x50}
        ,{0x05, 0x00, 0x00, 0x00, SNFC_N5_5_0_0_0, 0x50}
        ,{0x05, 0x00, 0x00, 0x01, SNFC_N5_5_0_0_1, 0x30}
        ,{0x05, 0x00, 0x00, 0x02, SNFC_N5_5_0_x_2, 0x30}
        ,{0x05, 0x00, 0x00, 0x03, SNFC_N5P_5_0_x_3, 0x30}
        ,{0x05, 0x00, 0x00, 0x05, SNFC_N5P_5_0_x_5, 0x30}
        ,{0x05, 0x00, 0x00, 0x06, SNFC_N5P_5_0_x_6, 0x00}
        ,{0x05, 0x00, 0x00, 0x07, SNFC_N5P_5_0_x_7, 0x30}
        ,{0x05, 0x00, 0x00, 0x08, SNFC_N5S_5_0_x_8, 0x30}
        };

static uint8_t _n7_bl_ver_list[][6] = {
        {0x00, 0x00, 0x00, 0x00, SNFC_UNKNOWN, 0x30}
        ,{0x07, 0x00, 0x00, 0x00, SNFC_N7, 0x30}
        ,{0x07, 0x00, 0x00, 0x00, SNFC_N7_7_0_0_x, 0x30}
        };

static uint8_t _n8_bl_ver_list[][6] = {
        {0x00, 0x00, 0x00, 0x00, SNFC_UNKNOWN, 0x00}
        ,{0x80, 0x00, 0x00, 0x00, SNFC_N8, 0x00}
        ,{0x81, 0x00, 0x00, 0x00, SNFC_N81, 0x00}
		,{0x82, 0x00, 0x00, 0x00, SNFC_N82, 0x00}
        };

static eNFC_SNFC_PRODUCTS product_map(uint8_t *ver, uint16_t *base_address)
{
    size_t i;
    for (i=0; i<sizeof(_n3_bl_ver_list)/6; i++)
    {
        if (ver[0] == _n3_bl_ver_list[i][0]
            && ver[1] == _n3_bl_ver_list[i][1]
            && ver[2] == _n3_bl_ver_list[i][2]
            && ver[3] == _n3_bl_ver_list[i][3])
        {
            *base_address = _n3_bl_ver_list[i][5] * 0x100;
            return _n3_bl_ver_list[i][4];
        }
    }

    for (i=0; i<sizeof(_n5_bl_ver_list)/6; i++)
    {
        if (ver[0] == _n5_bl_ver_list[i][0]
            && ver[1] == _n5_bl_ver_list[i][1]
            //&& ver[2] == _n5_bl_ver_list[i][2]  // Customer key
            && ver[3] == _n5_bl_ver_list[i][3])
        {
            *base_address = _n5_bl_ver_list[i][5] * 0x100;
            return _n5_bl_ver_list[i][4];
        }
    }

    for (i=0; i<sizeof(_n7_bl_ver_list)/6; i++)
    {
        if (ver[0] == _n7_bl_ver_list[i][0]
            && ver[1] == _n7_bl_ver_list[i][1]
            //&& ver[2] == _n7_bl_ver_list[i][2]  // Customer key
            && ver[3] == _n7_bl_ver_list[i][3])
        {
            *base_address = _n7_bl_ver_list[i][5] * 0x100;
            return _n7_bl_ver_list[i][4];
        }
    }

    for (i=0; i<sizeof(_n8_bl_ver_list)/6; i++)
    {
        if (ver[0] == _n8_bl_ver_list[i][0]
            && ver[1] == _n8_bl_ver_list[i][1]
            //&& ver[2] == _n7_bl_ver_list[i][2]  // Customer key
            && ver[3] == _n8_bl_ver_list[i][3])
        {
            *base_address = _n8_bl_ver_list[i][5] * 0x100;
            return _n8_bl_ver_list[i][4];
        }
    }

    return SNFC_UNKNOWN;
}

static char *get_product_name(uint8_t product)
{
    int i;
    for (i=0; i<sizeof(_product_info)/sizeof(struct _sproduct_info); i++)
    {
        if (_product_info[i].group == productGroup(product))
        {
            if (product == 0x81)
            {
                if (sizeof(_product_info)/sizeof(struct _sproduct_info) <= i+1)
                    return "Unknown product";
                return _product_info[i+1].chip_name;
            }
            return _product_info[i].chip_name;
        }
    }

    return "Unknown product";
}

static uint8_t get_product_support_fw(uint8_t product)
{
    int i;
    for (i=0; i<sizeof(_product_info)/sizeof(struct _sproduct_info); i++)
    {
        if (_product_info[i].group == productGroup(product))
        {
            if (product == 0x81)
            {
                if (sizeof(_product_info)/sizeof(struct _sproduct_info) <= i+1)
                    return SNFC_UNKNOWN;
                return _product_info[i+1].major_version;
            }
            return _product_info[i].major_version;
        }
    }

    return SNFC_UNKNOWN;
}
#endif //__NFC_SEC_PRODUCT__
