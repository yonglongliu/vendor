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
#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_THREAD_NUM	4

#define MULTI_THREAD

typedef struct worker
{
	void *(*process) (void *arg);
	void *arg;
	struct worker *next;

} CThread_worker;

typedef struct {
	int core;
	void *pool;
} thread_arg;

typedef struct
{
	pthread_mutex_t queue_lock;
	pthread_cond_t  queue_ready;
	pthread_mutex_t pro_lock;
	pthread_cond_t  pro_ready;

	CThread_worker *queue_head;
	int shutdown;
	pthread_t *threadid;
	int *thread_createflag;
	int max_thread_num;
	int cur_queue_size;
	pthread_attr_t  *attr;

	int thread_done;

	thread_arg args[MAX_THREAD_NUM];
} CThread_pool;
#ifdef __cplusplus
extern "C" {
#endif
int sprd_pool_add_worker(void *pool, void *(*process) (void *arg), void *arg);
int sprd_pool_init(CThread_pool **pool_ptr, int threadNum);
int sprd_pool_destroy(CThread_pool *pool);

void sprd_sync_thread_done(void *arg, int sync_num);
void sprd_reset_thread_done(void *arg);
void sprd_one_thread_done(void *arg);
#ifdef __cplusplus
}
#endif
#endif
