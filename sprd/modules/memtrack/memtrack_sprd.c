/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <hardware/memtrack.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define min(x, y) ((x) < (y) ? (x) : (y))

struct memtrack_record record_templates[] = {
    {
        .flags = MEMTRACK_FLAG_SMAPS_ACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
    {
        .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
};

#define LOG_TAG "memtrack_sprd"

#include <log/log.h>

static int get_gpu_memtrack_memory(pid_t pid, size_t *accounted, size_t *unaccounted)
{
	// GPU owner please add code;
	// *accounted = ;
	// *unaccounted = ;
	return 0;
}

static int get_ion_memtrack_memory(pid_t pid, size_t *accounted, size_t *unaccounted)
{
	// ION owner please add code;
	// *accounted = ;
	// *unaccounted = ;
	return 0;
}

static int sprd_get_memory(pid_t pid, enum memtrack_type type,
                             struct memtrack_record *records,
                             size_t *num_records)
{
	size_t allocated_records = min(*num_records, ARRAY_SIZE(record_templates));
	size_t accounted_size = 0;
	size_t unaccounted_size = 0;

	*num_records = ARRAY_SIZE(record_templates);

	ALOGD("sprd_get_memory, pid:%d, type:%d, allocated_records:%d, num records:%d",(int)pid, type, allocated_records, *num_records);

	/* fastpath to return the necessary number of records */
	if (allocated_records == 0) {
		return 0;
	}

	memcpy(records, record_templates,
		   sizeof(struct memtrack_record) * allocated_records);

	if (type == MEMTRACK_TYPE_GL) {
		 get_gpu_memtrack_memory(pid, &accounted_size, &unaccounted_size);
	} else if (type == MEMTRACK_TYPE_GRAPHICS) {
		 get_ion_memtrack_memory(pid, &accounted_size, &unaccounted_size);
	}

	if (allocated_records > 0) {
		records[0].size_in_bytes = accounted_size;
	}
	if (allocated_records > 1) {
		records[1].size_in_bytes = unaccounted_size;
	}

	return 0;
}

static int sprd_memtrack_init(const struct memtrack_module *module)
{
    return 0;
}

static 
int sprd_memtrack_get_memory(const struct memtrack_module *module,
                                pid_t pid,
                                int type,
                                struct memtrack_record *records,
                                size_t *num_records)
{
    if (type == MEMTRACK_TYPE_GL || type == MEMTRACK_TYPE_GRAPHICS) {
        return sprd_get_memory(pid, type, records, num_records);
    }

    return -EINVAL;
}

static struct hw_module_methods_t memtrack_module_methods = {
    .open = NULL,
};

struct memtrack_module HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version: MEMTRACK_MODULE_API_VERSION_0_1,
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: MEMTRACK_HARDWARE_MODULE_ID,
        name: "sprd Memory Tracker HAL",
        author: "The Android Open Source Project",
        methods: &memtrack_module_methods,
    },

    init: sprd_memtrack_init,
    getMemory: sprd_memtrack_get_memory,
};
