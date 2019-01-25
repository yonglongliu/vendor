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

#ifndef INCLUDE_POWER_UTILS_H
#define INCLUDE_POWER_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <linux/time.h>

#define ENTER(x,...)                    ALOGD("Enter %s: " x, __func__, ##__VA_ARGS__)
#define EXIT(x,...)                     ALOGD("Exit %s: " x, __func__, ##__VA_ARGS__)

#define STR(x)                          #x
#define MACOR_VALUE_TO_STR2(x)          STR(x)
#define _TOK_CONCAT(x,y)                x##y

#define SEC_TO_MS                       1000L
#define MS_TO_US                        1000L
#define MS_TO_NS                        1000000L

long long calc_timespan_ms(struct timespec start, struct timespec end);
void sprd_strftime(char *buf, int size, struct timespec ts);

void sprd_write(const char *path, const char *s);
int sprd_read(const char *path, char *s, int num_bytes);
int get_string_default_value(char *path, char *buf, int size);
int get_integer_default_value(char *path, unsigned int *value);

void sprd_timer_create(int signo, timer_t *timer_id, int tid);
void sprd_timer_settime(timer_t timer_id, long long value);
#endif
