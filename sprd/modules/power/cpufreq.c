/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include "utils.h"
#include "common.h"

int sprdemand_boost_set(int enable, int duration, const char *path, struct file *file)
{
    char buf[128] = {'\0'};

    if (CC_UNLIKELY(path == NULL || file == NULL))
        return 0;

    ENTER();
    snprintf(buf, sizeof(buf), "%s/%s", path, file->name);
    ALOGD("Set %s: 4", buf);
    sprd_write(buf, "4");

    return 1;
}
