/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Authors:
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SPRDWL_TCP_ACK_H__
#define __SPRDWL_TCP_ACK_H__

#include "msg.h"

#define SPRDWL_TCP_ACK_NUM  4
#define SPRDWL_TCP_ACK_CONTINUE_NUM	10
#define SPRDWL_TCP_ACK_DROP_CNT		6
#define SPRDWL_TCP_ACK_DROP_TIME	10
#define SPRDWL_TCP_ACK_EXIT_VAL		0x800

#define SPRDWL_ACK_OLD_TIME	4000
#define SPRDWL_U32_BEFORE(a, b)	((__s32)((__u32)a - (__u32)b) < 0)

#ifdef TCP_ACK_DROP_SUPPORT
extern unsigned int tcp_ack_drop_enable;
#else
extern const unsigned int tcp_ack_drop_enable;
#endif
extern unsigned int tcp_ack_drop_cnt;
struct sprdwl_tcp_ack_msg {
	u16 source;
	u16 dest;
	s32 saddr;
	s32 daddr;
	u32 seq;
};

struct sprdwl_tcp_ack_info {
	int ack_info_num;
	int busy;
	int drop_cnt;
	int psh_flag;
	u32 psh_seq;
	/* lock for ack info */
	spinlock_t lock;
	unsigned long last_time;
	unsigned long timeout;
	struct timer_list timer;
	struct sprdwl_msg_buf *msgbuf;
	struct sprdwl_tcp_ack_msg ack_msg;
};

struct sprdwl_tcp_ack_manage {
	/* 1 filter */
	int max_num;
	int free_index;
	unsigned long last_time;
	unsigned long timeout;
	unsigned long drop_time;
	atomic_t ref;
	/* lock for tcp ack alloc and free */
	spinlock_t lock;
	struct sprdwl_priv *priv;
	struct sprdwl_tcp_ack_info ack_info[SPRDWL_TCP_ACK_NUM];
};

void sprdwl_tcp_ack_init(struct sprdwl_priv *priv);
void sprdwl_tcp_ack_deinit(struct sprdwl_priv *priv);
void sprdwl_filter_rx_tcp_ack(struct sprdwl_priv *priv, unsigned char *buf);
/* return val: 0 for not fileter, 1 for fileter */
int sprdwl_filter_send_tcp_ack(struct sprdwl_priv *priv,
			       struct sprdwl_msg_buf *msgbuf,
			       unsigned char *buf);

#endif
