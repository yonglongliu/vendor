/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef GRALLOC_HELPER_H_
#define GRALLOC_HELPER_H_

#include <sys/mman.h>
#include "format_chooser.h"

inline size_t round_up_to_page_size(size_t x)
{
    return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}

/** @brief Maps an Android flexible YUV format to the underlying format.
 *
 * @param format The format, including the internal format extension bits.
 *
 * @returns The mapped format, without the internal format extension bits.
 */
inline uint64_t map_format( uint64_t format )
{
	/* Extended YUV in gralloc_arm_hal_index_format enum overlaps Android's HAL_PIXEL_FORMAT enum so check for this first */
	int extended_yuv = (format & GRALLOC_ARM_INTFMT_EXTENDED_YUV) == GRALLOC_ARM_INTFMT_EXTENDED_YUV;

	/* Lose the extension bits */
	format &= GRALLOC_ARM_INTFMT_FMT_MASK;

	if (!extended_yuv)
	{
		switch(format)
		{
		case HAL_PIXEL_FORMAT_YCbCr_420_888:
			format = GRALLOC_MAPPED_HAL_PIXEL_FORMAT_YCbCr_420_888;
			break;
		}
	}
	return format;
}

#endif /* GRALLOC_HELPER_H_ */
