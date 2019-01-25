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

#ifndef INCLUDE_SPRD_POWER_DEVFREQ_H
#define INCLUDE_SPRD_POWER_DEVFREQ_H

#include "common.h"

#define NUM_DEVFREQ_AVAILABLE_FREQ_MAX               6

#define PATH_DEVFREQ_DDR_BOOST                       "/sys/class/devfreq/scene-frequency/sprd_governor/scene_boost_dfs"
#define PATH_DEVFREQ_DDR_FREQ_TABLE                  "/sys/class/devfreq/scene-frequency/sprd_governor/ddrinfo_freq_table"

int devfreq_ddr_set(int enable, int duration, const char *path, struct file *file);
int devfreq_ddr_clear(const char *path, struct file *file);
#endif
