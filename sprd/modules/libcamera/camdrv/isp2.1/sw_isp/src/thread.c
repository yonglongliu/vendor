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
#include "thread.h"
#include <assert.h>
//#define CPU_BUNDLE
#ifdef CPU_BUNDLE
#include <sched.h>
#endif

typedef struct {
	int core;
	void *pool;
} thread_arg_t;

void sprd_sync_thread_done(void *arg, int sync_num)
{
	CThread_pool *pool = (CThread_pool *)arg;
	pthread_mutex_lock(&(pool->pro_lock));
	while (pool->thread_done != sync_num)
		pthread_cond_wait(&(pool->pro_ready), &(pool->pro_lock));
	pthread_mutex_unlock(&(pool->pro_lock));
}

void sprd_reset_thread_done(void *arg)
{
	CThread_pool *pool = (CThread_pool *)arg;
	pthread_mutex_lock(&(pool->pro_lock));
	pool->thread_done = 0;
	pthread_mutex_unlock(&(pool->pro_lock));
}

void sprd_one_thread_done(void *arg)
{
	CThread_pool *pool = (CThread_pool *)arg;
	pthread_mutex_lock (&(pool->pro_lock));
	pool->thread_done++;
	pthread_mutex_unlock (&(pool->pro_lock));
	pthread_cond_signal (&(pool->pro_ready));
}

void *thread_routine (void *arg_void)
{
	thread_arg *arg = (thread_arg *)arg_void;
	CThread_pool *pool = (CThread_pool *)arg->pool;
	CThread_worker *worker;
#ifdef CPU_BUNDLE
	cpu_set_t cpu_info;
	CPU_ZERO(&cpu_info);
	CPU_SET(arg->core, &cpu_info);
	if (sched_setaffinity(0, sizeof(cpu_info), &cpu_info) != 0)
		printf("sched_setaffinity failed");
#endif
	while (1)
	{
		pthread_mutex_lock (&(pool->queue_lock));

		while (pool->cur_queue_size == 0 && !pool->shutdown)
		{
			pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock));
		}

		if (pool->shutdown)
		{
			pthread_mutex_unlock (&(pool->queue_lock));

			pthread_exit (NULL);
		}

		assert (pool->cur_queue_size != 0);
		assert (pool->queue_head != NULL);

		pool->cur_queue_size--;

		worker = pool->queue_head;
		pool->queue_head = worker->next;
		pthread_mutex_unlock (&(pool->queue_lock));

		(*(worker->process)) (worker->arg);

		free (worker);
		worker = NULL;
	}

	return NULL;
}

int sprd_pool_init(CThread_pool **pool_ptr, int threadNum)
{
	int err;
	int i = 0;
	int failnum = 0;
	CThread_pool *pool = (CThread_pool *) malloc (sizeof (CThread_pool));
	int max_thread_num = threadNum;

	if(pool == NULL)
	{
		printf("can't alloc thread pool\n");
		goto __error;
	}

	*pool_ptr = pool;

	memset(pool , 0 , sizeof(CThread_pool));
	err = pthread_mutex_init (&(pool->queue_lock), NULL);
	if(err != 0)
	{
		printf("can't init pool->queue_lock err:%d\n" , err);
		goto __error;
	}
	err = pthread_cond_init (&(pool->queue_ready), NULL);
	if(err != 0)
	{
		printf("can't init pool->queue_ready err:%d\n" , err);
		goto __error;
	}

	err = pthread_mutex_init (&(pool->pro_lock), NULL);
	if(err != 0)
	{
		printf("can't init pool->pro_lock err:%d\n" , err);
		goto __error;
	}

	err = pthread_cond_init (&(pool->pro_ready), NULL);
	if(err != 0)
	{
		printf("can't init pool->pro_ready err:%d\n" , err);
		goto __error;
	}

	pool->queue_head = NULL;
	pool->max_thread_num = max_thread_num;
	pool->cur_queue_size = 0;
	pool->shutdown = 0;
	pool->threadid =
		(pthread_t *) malloc (max_thread_num * sizeof (pthread_t));
	if(NULL == pool->threadid)
	{
		printf("can't malloc threadid\n");
		goto __error;
	}
	pool->thread_createflag = (int*)malloc(max_thread_num*sizeof(int));
	if(NULL == pool->thread_createflag)
	{
		printf("can't malloc thread_creatflag\n");
		goto __error;
	}
	memset(pool->thread_createflag , 0 , max_thread_num*sizeof(int));
	memset(pool->threadid , 0 , max_thread_num * sizeof (pthread_t));

	for (i = 0; i < max_thread_num; i++)
	{
		pool->args[i].core = i;
		pool->args[i].pool = pool;
		err = pthread_create (&(pool->threadid[i]), NULL, thread_routine, &pool->args[i]);

		if (err!=0)
		{
			printf("can't create thread: %s\n", strerror(err));
			pool->thread_createflag[i] = 0;
			failnum++;
		}
		else
			pool->thread_createflag[i] = 1;
	}
	if(max_thread_num == failnum)
	{
		printf("all LTM thread create failed.\n");
		goto __error;
	}
	return 0;

__error:
	if(pool != NULL)
	{
		if(NULL != pool->threadid)
			free(pool->threadid);
		if(NULL != pool->thread_createflag)
			free(pool->thread_createflag);
		free(pool);
	}
	return -1;
}

int sprd_pool_add_worker (void *pool_void, void *(*process) (void *arg), void *arg)
{
	CThread_pool *pool = (CThread_pool *)pool_void;
	CThread_worker *member;
	CThread_worker *newworker =
		(CThread_worker *) malloc (sizeof (CThread_worker));
	newworker->process = process;
	newworker->arg = arg;
	newworker->next = NULL;

	pthread_mutex_lock (&(pool->queue_lock));

	member = pool->queue_head;
	if (member != NULL)
	{
		while (member->next != NULL)
			member = member->next;
		member->next = newworker;
	}
	else
	{
		pool->queue_head = newworker;
	}

	assert (pool->queue_head != NULL);

	pool->cur_queue_size++;
	pthread_mutex_unlock (&(pool->queue_lock));

	pthread_cond_signal (&(pool->queue_ready));

	return 0;
}

int sprd_pool_destroy (CThread_pool *pool)
{
	int i,err;
	int ret = 0;
	CThread_worker *head;
	if (pool->shutdown)
		return -1;
	pthread_mutex_lock (&(pool->queue_lock));
	pool->shutdown = 1;
	pthread_mutex_unlock (&(pool->queue_lock));
	pthread_cond_broadcast (&(pool->queue_ready));

	for (i = 0; i < pool->max_thread_num; i++)//
	{
		if(1 == pool->thread_createflag[i])
		{
			err=pthread_join(pool->threadid[i], NULL);
			if (err != 0)
			{
				printf("pthread_join fail: %s , i:%d , threadid:%ld\n", strerror(err) , i , pool->threadid[i]);
				ret = -1;
			}
		}
	}
	free (pool->threadid);
	free(pool->thread_createflag);
	head = NULL;
	while (pool->queue_head != NULL)
	{
		head = pool->queue_head;
		pool->queue_head = pool->queue_head->next;
		free (head);
	}

	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->queue_ready));
	pthread_mutex_destroy(&(pool->pro_lock));
	pthread_cond_destroy(&(pool->pro_ready));

	free (pool);

	pool=NULL;

	return ret;
}
