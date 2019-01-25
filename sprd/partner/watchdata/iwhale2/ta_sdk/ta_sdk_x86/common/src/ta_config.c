#include "tee_internal_api.h"

char* ta_uuid = TA_NAME;

static TEE_UUID smc_uuid = {
		0x534d4152,
		0x542d,
		0x4353,
		{0x4c, 0x54,0x2d, 0x54, 0x41, 0x2d, 0x30, 0x31}
};

static uint8_t smc_id[20] = {0x00, 0x00, 0x00, 0xF0,
		0x52, 0x41, 0x4d, 0x53, 
		0x2d, 0x54, 
		0x53, 0x43, 
		0x4C, 0x54, 0x2d, 0x54, 0x41, 0x2d, 0x30, 0x31
};
static __TEE_EX_PROPERTY  ex_property[] = {
	{
			"smc.ta.teststring",
			TYPE_STRING,
			{
					.memref.buffer = "this is a test string",
					.memref.size = sizeof("this is a test string") 
			},
	},
	{
			"smc.ta.testbooltrue",
			TYPE_BOOLEAN,
			{
					.value.a = 1,
			}
	},
	{
			"smc.ta.testboolfalse",
			TYPE_BOOLEAN,
			{
					.value.a = 0,
			}
	},
	{
			"smc.ta.testu32",
			TYPE_INTEGER,
			{
					.value.a = 48059,
			}
	},
	{
			"smc.ta.testbinaryblock",
			TYPE_STRING,
			{
					//.memref.buffer = "VGhpcyBpcyBhIHRleHQgYmluYXJ5IGJsb2Nr",
					//.memref.size = sizeof("VGhpcyBpcyBhIHRleHQgYmluYXJ5IGJsb2Nr") 
					.memref.buffer = "This is a text binary block",
					.memref.size = sizeof("This is a text binary block") -1
			}
	},
	{
			"smc.ta.testuuid",
			TYPE_UUID,
			{
					.memref.buffer = &smc_uuid,
					.memref.size = 16,
			}
	},
	{
			"smc.ta.testidentity",
			TYPE_IDENTITY,
			{
					.memref.buffer = smc_id,
					.memref.size = 20,
			}
	}
};
TA_REG ta_reg = {
		//uuid
		{ 0x534D4152, 0x542D, 0x4353, {0x4C, 0x54, 0x2D, 0x54, 0x41, 0x2D, 0x30, 0x31}},
		//isSingleInstance
		true,
		//isMultiSession
		false,
		//isInstanceKeepAlive
		false,
		//dataSize
		128*1024,
		//stackSize
		16*1024,
        //max_ex_item:扩展属性的个数，目前仅在TCF上有需求
		7,
		//__TEE_EX_PROPERTY
	    ex_property
};
