/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors	:
 * Jingxiang Li <Jingxiang.li@spreadtrum.com>
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

#include <linux/utsname.h>

#include <linux/sdiom_rx_api.h>
#include <linux/sdiom_tx_api.h>
#include <linux/debugfs.h>

#include "sprdwl.h"
#include "sdio.h"
#include "msg.h"
#include "txrx.h"
#include "qos.h"

#define SPRDWL_TX_CMD_TIMEOUT	3000
#define SPRDWL_TX_DATA_TIMEOUT	4000
#define SPRDWL_WAKE_TIMEOUT	240
#define SPRDWL_WAKE_PRE_TIMEOUT	80

#define SPRDWL_SDIOM_WIFI 2
enum sprdwl_sdiom_port {
	/* cp port 8 */
	SPRDWL_SDIOM_CMD_TX,
	/* cp port 8 */
	SPRDWL_SDIOM_CMD_RX = 0,
	/* cp port 9 */
	SPRDWL_SDIOM_DATA_TX = 1,
	/* cp port 9 */
	SPRDWL_SDIOM_DATA_RX = 1,
};

#define SPRDWL_TX_MSG_CMD_NUM 2
#define SPRDWL_TX_MSG_DATA_NUM 192
#define SPRDWL_TX_DATA_START_NUM (SPRDWL_TX_MSG_DATA_NUM - 3)
#define SPRDWL_RX_MSG_NUM 128

#define SPRDWL_SDIO_MASK_LIST0	0x1
#define SPRDWL_SDIO_MASK_LIST1	0x2
#define SPRDWL_SDIO_MASK_LIST2	0x4

static struct sprdwl_intf *p_sprdwl_intf;

static void sprdwl_sdio_wakeup_tx(void);

static void sprdwl_sdio_wake_net_ifneed(struct sprdwl_intf *intf,
					struct sprdwl_msg_list *list,
					enum sprdwl_mode mode)
{
	if (atomic_read(&list->flow)) {
		if (atomic_read(&list->ref) <= SPRDWL_TX_DATA_START_NUM) {
			atomic_set(&list->flow, 0);
			intf->net_start_cnt++;
			if (intf->driver_status)
				sprdwl_net_flowcontrl(intf->priv, mode, true);
		}
	}
}

static int sprdwl_sdio_txmsg(struct sprdwl_intf *intf,
				struct sprdwl_msg_buf *msg)
{
	u16 len;
	unsigned char *info;

	if (msg->msglist == &intf->tx_list0) {
		len = SPRDWL_MAX_CMD_TXLEN;
		info = "cmd";
		msg->timeout = jiffies + intf->cmd_timeout;
	} else {
		len = SPRDWL_MAX_DATA_TXLEN;
		info = "data";
		msg->timeout = jiffies + intf->data_timeout;
	}

	if (msg->len > len) {
		dev_err(&intf->pdev->dev,
			"%s err:%s too long:%d > %d,drop it\n",
			__func__, info, msg->len, len);
		WARN_ON(1);
		dev_kfree_skb(msg->skb);
		sprdwl_free_msg_buf(msg, msg->msglist);
		return 0;
	}

	sprdwl_queue_msg_buf(msg, msg->msglist);
	sprdwl_sdio_wakeup_tx();
	queue_work(intf->tx_queue, &intf->tx_work);

	return 0;
}

static struct
sprdwl_msg_buf *sprdwl_sdio_get_msg_buf(struct sprdwl_intf *intf,
					enum sprdwl_head_type type,
					enum sprdwl_mode mode)
{
	u8 sdiom_type;
	struct sprdwl_msg_buf *msg;
	struct sprdwl_msg_list *list;

	if (unlikely(intf->exit))
		return NULL;
	if (type == SPRDWL_TYPE_DATA) {
		sdiom_type = SPRDWL_SDIOM_DATA_TX;
		if (mode <= SPRDWL_MODE_AP)
			list = &intf->tx_list1;
		else
			list = &intf->tx_list2;
	} else {
		sdiom_type = SPRDWL_SDIOM_CMD_TX;
		list = &intf->tx_list0;
	}
	msg = sprdwl_alloc_msg_buf(list);
	if (msg) {
		msg->type = sdiom_type;
		msg->msglist = list;
		msg->mode = mode;
		return msg;
	}

	if (type == SPRDWL_TYPE_DATA) {
		intf->net_stop_cnt++;
		sprdwl_net_flowcontrl(intf->priv, mode, false);
		atomic_set(&list->flow, 1);
	}
	printk_ratelimited("%s no more msgbuf for %s\n",
			   __func__, type == SPRDWL_TYPE_DATA ?
			   "data" : "cmd");

	return NULL;
}

static void sprdwl_sdio_free_msg_buf(struct sprdwl_intf *intf,
				struct sprdwl_msg_buf *msg)
{
	sprdwl_free_msg_buf(msg, msg->msglist);
}

static void sprdwl_sdio_force_exit(void)
{
	p_sprdwl_intf->exit = 1;
	wake_up_all(&p_sprdwl_intf->waitq);
}

static int sprdwl_sdio_is_exit(void)
{
	return p_sprdwl_intf->exit;
}

#define SPRDWL_SDIO_DEBUG_BUFLEN 128
static ssize_t sprdwl_sdio_read_info(struct file *file,
				     char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	unsigned int buflen, len;
	size_t ret;
	unsigned char *buf;
	struct sprdwl_intf *intf;

	intf = (struct sprdwl_intf *)file->private_data;
	buflen = SPRDWL_SDIO_DEBUG_BUFLEN;
	buf = kzalloc(buflen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len = 0;
	len += scnprintf(buf, buflen, "net: stop %u, start %u\n"
			 "drop cnt: cmd %u, sta %u, p2p %u\n"
			 "ring_ap:%u ring_cp:%u common:%u sta:%u p2p:%u\n",
			 intf->net_stop_cnt, intf->net_start_cnt,
			 intf->drop_cmd_cnt, intf->drop_data1_cnt,
			 intf->drop_data2_cnt,
			 intf->ring_ap, intf->ring_cp,
			 atomic_read(&intf->flow0),
			 atomic_read(&intf->flow1),
			 atomic_read(&intf->flow2));
	if (len > buflen)
		len = buflen;

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return ret;
}

static ssize_t sprdwl_sdio_write(struct file *file,
				 const char __user *__user_buf,
				 size_t count, loff_t *ppos)
{
	char buf[20];

	if (!count || count >= sizeof(buf)) {
		dev_err(&p_sprdwl_intf->pdev->dev,
			"write len too long:%zu >= %u\n", count,
			(unsigned int)sizeof(buf));
		return -EINVAL;
	}
	if (copy_from_user(buf, __user_buf, count))
		return -EFAULT;
	buf[count] = '\0';
	dev_err(&p_sprdwl_intf->pdev->dev, "write info:%s\n", buf);

	return count;
}

static const struct file_operations sprdwl_sdio_debug_fops = {
	.read = sprdwl_sdio_read_info,
	.write = sprdwl_sdio_write,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static void sprdwl_sdio_debugfs(struct dentry *dir)
{
	debugfs_create_file("sdioinfo", S_IRUSR,
			    dir, p_sprdwl_intf, &sprdwl_sdio_debug_fops);
}

static void sprdwl_sdio_tcp_drop_msg(struct sprdwl_msg_buf *msgbuf)
{
	enum sprdwl_mode mode;
	struct sprdwl_msg_list *list;

	dev_kfree_skb(msgbuf->skb);
	mode = msgbuf->mode;
	list = msgbuf->msglist;
	sprdwl_free_msg_buf(msgbuf, list);
	sprdwl_sdio_wake_net_ifneed(p_sprdwl_intf, list, mode);
}

static void sprdwl_sdio_set_qos(enum sprdwl_mode mode, int enable)
{
	struct sprdwl_msg_list *list;

	if (mode <= SPRDWL_MODE_AP)
		list = &p_sprdwl_intf->tx_list1;
	else
		list = &p_sprdwl_intf->tx_list2;
	dev_info(&p_sprdwl_intf->pdev->dev, "%s set qos:%d\n",
		 __func__, enable);
	list->qos.enable = enable ? 1 : 0;
	list->qos.change = 1;
}

static int sprdwl_sdio_reserv_len(void)
{
	return 0;
}

struct sprdwl_if_ops sprdwl_sdio_ops = {
	.get_msg_buf = sprdwl_sdio_get_msg_buf,
	.free_msg_buf = sprdwl_sdio_free_msg_buf,
	.tx = sprdwl_sdio_txmsg,
	.force_exit = sprdwl_sdio_force_exit,
	.is_exit = sprdwl_sdio_is_exit,
	.debugfs = sprdwl_sdio_debugfs,
	.tcp_drop_msg = sprdwl_sdio_tcp_drop_msg,
	.set_qos = sprdwl_sdio_set_qos,
	.reserv_len = sprdwl_sdio_reserv_len,
};

#ifdef SPRDWL_RX_NO_WORKQUEUE
static void sprdwl_rx_process(unsigned char *data, unsigned int len,
			      unsigned int fifo_id)
{
	struct sprdwl_priv *priv;
	struct sprdwl_intf *intf;

	intf = p_sprdwl_intf;
	priv = intf->priv;

	switch (SPRDWL_HEAD_GET_TYPE(data)) {
	case SPRDWL_TYPE_DATA:
		if (len > SPRDWL_MAX_DATA_RXLEN)
			dev_err(&intf->pdev->dev,
				"err rx data too long:%d > %d\n",
				len, SPRDWL_MAX_DATA_RXLEN);
		sprdwl_rx_data_process(priv, data);
		break;
	case SPRDWL_TYPE_CMD:
		if (len > SPRDWL_MAX_CMD_RXLEN)
			dev_err(&intf->pdev->dev,
				"err rx cmd too long:%d > %d\n",
				len, SPRDWL_MAX_CMD_RXLEN);
		sprdwl_rx_rsp_process(priv, data);
		break;
	case SPRDWL_TYPE_EVENT:
		if (len > SPRDWL_MAX_CMD_RXLEN)
			dev_err(&intf->pdev->dev,
				"err rx event too long:%d > %d\n",
				len, SPRDWL_MAX_CMD_RXLEN);
		sprdwl_rx_event_process(priv, data);
		break;
	default:
		dev_err(&intf->pdev->dev, "rx unkonow type:%d\n",
			SPRDWL_HEAD_GET_TYPE(data));
			break;
	}
	sdiom_pt_read_release(fifo_id);
}
#else
static void sprdwl_rx_work_queue(struct work_struct *work)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_priv *priv;
	struct sprdwl_intf *intf;

	intf = container_of(work, struct sprdwl_intf, rx_work);
	priv = intf->priv;

	while ((msg = sprdwl_peek_msg_buf(&intf->rx_list))) {
		if (intf->exit)
			goto next;
		switch (SPRDWL_HEAD_GET_TYPE(msg->tran_data)) {
		case SPRDWL_TYPE_DATA:
			if (msg->len > SPRDWL_MAX_DATA_RXLEN)
				dev_err(&intf->pdev->dev,
					"err rx data too long:%d > %d\n",
					msg->len, SPRDWL_MAX_DATA_RXLEN);
			sprdwl_rx_data_process(priv, msg->tran_data);
			break;
		case SPRDWL_TYPE_CMD:
			if (msg->len > SPRDWL_MAX_CMD_RXLEN)
				dev_err(&intf->pdev->dev,
					"err rx cmd too long:%d > %d\n",
					msg->len, SPRDWL_MAX_CMD_RXLEN);
			sprdwl_rx_rsp_process(priv, msg->tran_data);
			break;
		case SPRDWL_TYPE_EVENT:
			if (msg->len > SPRDWL_MAX_CMD_RXLEN)
				dev_err(&intf->pdev->dev,
					"err rx event too long:%d > %d\n",
					msg->len, SPRDWL_MAX_CMD_RXLEN);
			sprdwl_rx_event_process(priv, msg->tran_data);
			break;
		default:
			dev_err(&intf->pdev->dev, "rx unkonow type:%d\n",
				SPRDWL_HEAD_GET_TYPE(msg->tran_data));
			break;
		}
next:
		sdiom_pt_read_release(msg->fifo_id);
		sprdwl_dequeue_msg_buf(msg, &intf->rx_list);
	};
}
#endif

#ifdef SPRDWL_TX_SELF
#define SPRDWL_SELF_TX_LEN	(1024 * 8)
#include <linux/marlin_platform.h>
static int sprdwl_tx_buf_init(struct sprdwl_tx_buf *txbuf, unsigned int size)
{
	txbuf->base = kmalloc(size, GFP_ATOMIC);
	if (txbuf->base) {
		txbuf->buf_len = size;
		txbuf->curpos = 0;
		txbuf->change_size = 0;
		return 0;
	}
	txbuf->base = kmalloc(4096, GFP_ATOMIC);
	if (txbuf->base) {
		txbuf->buf_len = 4096;
		txbuf->curpos = 0;
		txbuf->change_size = 1;
		pr_err("%s alloc tx buf err:size:%u err! temp use 4096\n",
		       __func__, size);
		return 0;
	}
	pr_err("%s alloc tx buf err:size:4096\n", __func__);
	return -ENOMEM;
}

static void sprdwl_tx_buf_deinit(struct sprdwl_tx_buf *txbuf)
{
	kfree(txbuf->base);
}

#include <asm/byteorder.h>
struct sprdwl_sdiom_puh {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned int pad:7;
	unsigned int len:16;
	unsigned int eof:1;
	unsigned int subtype:4;
	unsigned int type:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	unsigned int type:4;
	unsigned int subtype:4;
	unsigned int eof:1;
	unsigned int len:16;
	unsigned int pad:7;
#else
#error  "check <asm/byteorder.h> defines"
#endif
}; /* 32bits public header */

#define ALIGN_4BYTE(a) (((a) + 3) & ~3)
#define ALIGN_512BYTE(a) (((a) + 511) & ~511)

static int sprdwl_packet_txinfo(struct sprdwl_msg_buf *msg_buf,
				struct sprdwl_tx_buf *txbuf)
{
	unsigned short len;
	struct sprdwl_sdiom_puh *puh;

	len = ALIGN_4BYTE(msg_buf->len + sizeof(*puh));
	if (len + sizeof(*puh) > txbuf->buf_len - txbuf->curpos)
		return -ENOMEM;

	puh = (struct sprdwl_sdiom_puh *)(txbuf->base + txbuf->curpos);
	puh->type = SPRDWL_SDIOM_WIFI;
	puh->subtype = msg_buf->type;
	puh->len = msg_buf->len;
	puh->eof = 0;
	puh->pad = 0;

	memcpy(puh + 1, msg_buf->tran_data, msg_buf->len);
	txbuf->curpos += len;

	return 0;
}

static int sprdwl_send_packet(struct sprdwl_intf *intf,
			      struct sprdwl_tx_buf *txbuf, int retry)
{
	int ret;
	struct sprdwl_sdiom_puh *puh;

	if (txbuf->curpos == 0)
		return 0;

	puh = (struct sprdwl_sdiom_puh *)(txbuf->base + txbuf->curpos);
	puh->type = 0;
	puh->subtype = 0;
	puh->len = 0;
	puh->eof = 1;
	puh->pad = 0;
	txbuf->curpos += sizeof(*puh);
	txbuf->curpos = ALIGN_512BYTE(txbuf->curpos);

	ret = sdiom_sdio_pt_write_raw(txbuf->base, txbuf->curpos, 0);
	if (ret < 0) {
		dev_err(&intf->pdev->dev, "%s sdiom_sdio_pt_write_raw erro:%d\n",
			__func__, ret);
		txbuf->curpos = 0;
		return -1;
	}

	txbuf->curpos = 0;
	if (txbuf->change_size) {
		unsigned char *buf;

		buf = kmalloc(SPRDWL_SELF_TX_LEN, GFP_ATOMIC);
		if (buf) {
			kfree(txbuf->base);
			txbuf->base = buf;
			txbuf->buf_len = SPRDWL_SELF_TX_LEN;
			txbuf->change_size = 0;
			dev_err(&intf->pdev->dev, "%s chage tx buf to:%d\n",
				__func__, SPRDWL_SELF_TX_LEN);
		}
	}

	return 0;
}
#endif

static int sprdwl_tx_cmd(struct sprdwl_intf *intf, struct sprdwl_msg_list *list)
{
	int ret;
	struct sprdwl_msg_buf *msgbuf;

	while ((msgbuf = sprdwl_peek_msg_buf(list))) {
		if (unlikely(intf->exit)) {
			dev_kfree_skb(msgbuf->skb);
			sprdwl_dequeue_msg_buf(msgbuf, list);
			continue;
		}
		if (time_after(jiffies, msgbuf->timeout)) {
			intf->drop_cmd_cnt++;
			dev_err(&intf->pdev->dev,
				"tx drop cmd msg,dropcnt:%u\n",
				intf->drop_cmd_cnt);
			dev_kfree_skb(msgbuf->skb);
			sprdwl_dequeue_msg_buf(msgbuf, list);
			continue;
		}
#ifdef SPRDWL_TX_SELF
		ret = sprdwl_packet_txinfo(msgbuf, &intf->txbuf);
		if (!ret) {
			dev_kfree_skb(msgbuf->skb);
			sprdwl_dequeue_msg_buf(msgbuf, list);
		} else {
			dev_err(&intf->pdev->dev, "%s err:%d\n", __func__, ret);
			dev_kfree_skb(msgbuf->skb);
			sprdwl_dequeue_msg_buf(msgbuf, list);
			break;
		}
#else
		ret = sdiom_pt_write_skb(msgbuf->tran_data, msgbuf->skb,
					 msgbuf->len, SPRDWL_SDIOM_WIFI,
					 msgbuf->type);
		if (!ret)
			sprdwl_dequeue_msg_buf(msgbuf, list);
		else {
			dev_err(&intf->pdev->dev, "%s err:%d\n", __func__, ret);
			/* fixme if need retry */
			dev_kfree_skb(msgbuf->skb);
			sprdwl_dequeue_msg_buf(msgbuf, list);
		}
#endif
	}

#ifdef SPRDWL_TX_SELF
	sprdwl_send_packet(intf, &intf->txbuf, 0);
#endif
	return 0;
}

static int sprdwl_tx_data(struct sprdwl_intf *intf,
			  struct sprdwl_msg_list *list, atomic_t *flow)
{
	unsigned char mode;
	int ret, pkts;
	/* sendnum0 shared */
	int sendnum0, sendnum1;
	int sendcnt0 = 0;
	int sendcnt1 = 0;
	int *psend;
	unsigned int cnt;
	struct sprdwl_msg_buf *msgbuf;
	struct sprdwl_data_hdr *hdr;

	/* cp: self free link */
	sendnum0 = atomic_read(flow);
	/* ap: shared free link */
	sendnum1 = atomic_read(&intf->flow0);
	if (!sendnum0 && !sendnum1)
		return 0;
	pkts = -1;
	sprdwl_qos_reorder(&list->qos);
	while ((msgbuf = sprdwl_qos_peek_msg(&list->qos, &pkts))) {
		if (unlikely(intf->exit)) {
			dev_kfree_skb(msgbuf->skb);
			sprdwl_qos_update(&list->qos, msgbuf, &msgbuf->list);
			sprdwl_dequeue_msg_buf(msgbuf, list);
			sprdwl_need_resch(&list->qos);
			continue;
		}
		if (time_after(jiffies, msgbuf->timeout)) {
			char *pinfo;

			if (list == &intf->tx_list1) {
				pinfo = "data1";
				cnt = intf->drop_data1_cnt++;
			} else {
				pinfo = "data2";
				cnt = intf->drop_data2_cnt++;
			}
			dev_err(&intf->pdev->dev,
				"tx drop %s msg,dropcnt:%u\n", pinfo, cnt);
			dev_kfree_skb(msgbuf->skb);
			mode = msgbuf->mode;
			sprdwl_qos_update(&list->qos, msgbuf, &msgbuf->list);
			sprdwl_dequeue_msg_buf(msgbuf, list);
			sprdwl_need_resch(&list->qos);
			sprdwl_sdio_wake_net_ifneed(intf, list, mode);
			continue;
		}
		if (sendnum1) {
			sendnum1--;
			psend = &sendcnt1;
			hdr = (struct sprdwl_data_hdr *)msgbuf->tran_data;
			hdr->flow3 = 0;
		} else if (sendnum0) {
			sendnum0--;
			psend = &sendcnt0;
			hdr = (struct sprdwl_data_hdr *)msgbuf->tran_data;
			hdr->flow3 = 1;
		} else {
			break;
		}

#ifdef SPRDWL_TX_SELF
		ret = sprdwl_packet_txinfo(msgbuf, &intf->txbuf);
		if (!ret) {
			*psend += 1;
			dev_kfree_skb(msgbuf->skb);
			mode = msgbuf->mode;
			sprdwl_qos_update(&list->qos, msgbuf, &msgbuf->list);
			sprdwl_dequeue_msg_buf(msgbuf, list);
			sprdwl_need_resch(&list->qos);
			sprdwl_sdio_wake_net_ifneed(intf, list, mode);
		} else
			break;
#else
		ret = sdiom_pt_write_skb(msgbuf->tran_data, msgbuf->skb,
					 msgbuf->len, SPRDWL_SDIOM_WIFI,
					 msgbuf->type);
		if (!ret) {
			*psend += 1;
			mode = msgbuf->mode;
			sprdwl_qos_update(&list->qos, msgbuf, &msgbuf->list);
			sprdwl_dequeue_msg_buf(msgbuf, list);
			sprdwl_need_resch(&list->qos);
			sprdwl_sdio_wake_net_ifneed(intf, list, mode);
		} else {
			printk_ratelimited("%s pt_write_skb err:%d\n",
					   __func__, ret);
			usleep_range(800, 1000);
			break;
		}
#endif
	}

#ifdef SPRDWL_TX_SELF
	sprdwl_send_packet(intf, &intf->txbuf, 0);
#endif
	intf->ring_ap += sendcnt0 + sendcnt1;
	if (sendcnt1)
		atomic_sub(sendcnt1, &intf->flow0);
	if (sendcnt0)
		atomic_sub(sendcnt0, flow);

	return sendcnt0 + sendcnt1;
}

static void sprdwl_sdio_flush_txlist(struct sprdwl_msg_list *list)
{
	struct sprdwl_msg_buf *msgbuf;

	while ((msgbuf = sprdwl_peek_msg_buf(list))) {
		dev_kfree_skb(msgbuf->skb);
		sprdwl_dequeue_msg_buf(msgbuf, list);
		continue;
	}
}

static void sprdwl_sdio_flush_all_txlist(struct sprdwl_intf *intf)
{
	sprdwl_sdio_flush_txlist(&intf->tx_list0);
	sprdwl_sdio_flush_txlist(&intf->tx_list1);
	sprdwl_sdio_flush_txlist(&intf->tx_list2);
}

static void sprdwl_sdio_keep_wakeup(struct sprdwl_intf *intf)
{
	if (time_after(jiffies, intf->wake_last_time)) {
		intf->wake_last_time = jiffies + intf->wake_pre_timeout;
		wake_lock_timeout(&intf->keep_wake, intf->wake_timeout);
	}
}

static void sprdwl_tx_work_queue(struct work_struct *work)
{
	unsigned int send_list, needsleep;
	struct sprdwl_intf *intf;
	int send_num0, send_num1, send_num2;
#ifdef SPRDWL_TX_SELF
	int wake_state = 0;
#endif

	send_num0 = 0;
	send_num1 = 0;
	send_num2 = 0;
	intf = container_of(work, struct sprdwl_intf, tx_work);
RETRY:
	if (unlikely(intf->exit)) {
#ifdef SPRDWL_TX_SELF
		if (wake_state) {
			marlin_set_sleep(MARLIN_WIFI, TRUE);
			wake_unlock(&intf->tx_wakelock);
		}
#endif
		sprdwl_sdio_flush_all_txlist(intf);
		return;
	}
	send_list = 0;
	needsleep = 0;
	intf->do_tx = 0;
	if (sprdwl_msg_tx_pended(&intf->tx_list0))
		send_list |= SPRDWL_SDIO_MASK_LIST0;

	if (intf->driver_status) {
		if (sprdwl_msg_tx_pended(&intf->tx_list1)) {
			send_num0 = atomic_read(&intf->flow0);
			send_num1 = atomic_read(&intf->flow1);
			if (send_num1 || send_num0)
				send_list |= SPRDWL_SDIO_MASK_LIST1;
			else
				needsleep |= SPRDWL_SDIO_MASK_LIST1;
		}
		if (sprdwl_msg_tx_pended(&intf->tx_list2)) {
			send_num0 = atomic_read(&intf->flow0);
			send_num2 = atomic_read(&intf->flow2);
			if (send_num2 || send_num0)
				send_list |= SPRDWL_SDIO_MASK_LIST2;
			else
				needsleep |= SPRDWL_SDIO_MASK_LIST2;
		}
	}

	/*send_list = 0 & needsleep = 0 means tx_list is empty!
	 *send_list = 0 & needsleep != 0 means cp hasn't enough space for new message!
	 */
	if (!send_list) {
		if (!needsleep) {
#ifdef SPRDWL_TX_SELF
			if (wake_state) {
				marlin_set_sleep(MARLIN_WIFI, TRUE);
				wake_unlock(&intf->tx_wakelock);
			}
#endif
			sprdwl_sdio_keep_wakeup(intf);
			return;
		}
		printk_ratelimited("%s need sleep  -- 0x%x %d %d %d\n",
				   __func__, needsleep, send_num0,
				   send_num1, send_num2);
#ifdef SPRDWL_TX_SELF
		if (wake_state) {
			marlin_set_sleep(MARLIN_WIFI, TRUE);
			wake_unlock(&intf->tx_wakelock);
			wake_state = 0;
		}
#endif
		sprdwl_sdio_keep_wakeup(intf);
		wait_event(intf->waitq,
			   (intf->do_tx || intf->exit));
		goto RETRY;
	}
#ifdef SPRDWL_TX_SELF
	/*send list is not empty and cp has more buff, then not permit sleep!
	 */
	if (!wake_state) {
		int count = 0;

		wake_state = 1;
		wake_lock(&intf->tx_wakelock);
		while (!sdiom_resume_wait_status()) {
			if (unlikely(intf->exit))
				goto RETRY;
			usleep_range(2000, 3000);
			if (count++ >= 500) {
				dev_err(&intf->pdev->dev,
					"%s wait resume erro\n",
					__func__);
				count = 0;
			}
		}
		marlin_set_sleep(MARLIN_WIFI, FALSE);
		marlin_set_wakeup(MARLIN_WIFI);
	}
#endif

	if (send_list & SPRDWL_SDIO_MASK_LIST0)
		sprdwl_tx_cmd(intf, &intf->tx_list0);
	if (intf->driver_status) {
		if (send_list & SPRDWL_SDIO_MASK_LIST2)
			sprdwl_tx_data(intf, &intf->tx_list2, &intf->flow2);
		if (send_list & SPRDWL_SDIO_MASK_LIST1)
			sprdwl_tx_data(intf, &intf->tx_list1, &intf->flow1);
	}

	goto RETRY;
}

static void sprdwl_sdio_wakeup_tx(void)
{
	p_sprdwl_intf->do_tx = 1;
	wake_up(&p_sprdwl_intf->waitq);
}

static int sprdwl_sdio_process_credit(void *data)
{
	int ret = 0;
	unsigned char *flow;
	struct sprdwl_common_hdr *common;

	common = (struct sprdwl_common_hdr *)data;
	if (common->rsp && common->type == SPRDWL_TYPE_DATA) {
		flow = &((struct sprdwl_data_hdr *)data)->flow0;
		goto out;
	} else if (common->type == SPRDWL_TYPE_EVENT) {
		struct sprdwl_cmd_hdr *cmd;

		cmd = (struct sprdwl_cmd_hdr *)data;
		if (cmd->cmd_id == WIFI_EVENT_SDIO_FLOWCON) {
			flow = cmd->paydata;
			ret = -1;
			goto out;
		}
	}
	return 0;

out:
	if (flow[0])
		atomic_add(flow[0], &p_sprdwl_intf->flow0);
	if (flow[1])
		atomic_add(flow[1], &p_sprdwl_intf->flow1);
	if (flow[2])
		atomic_add(flow[2], &p_sprdwl_intf->flow2);
	if (flow[0] || flow[1] || flow[2]) {
		p_sprdwl_intf->ring_cp += flow[0] + flow[1] + flow[2];
		sprdwl_sdio_wakeup_tx();
	}

	return ret;
}

unsigned int sprdwl_sdio_rx_handle(void *data, unsigned int len,
				   unsigned int fifo_id)
{
#ifndef SPRDWL_RX_NO_WORKQUEUE
	struct sprdwl_msg_buf *msg;
#endif

	if (!data || !len) {
		dev_err(&p_sprdwl_intf->pdev->dev,
			"%s param erro:%p %d\n", __func__, data, len);
		goto out;
	}

	if (unlikely(p_sprdwl_intf->exit))
		goto out;

	if (sprdwl_sdio_process_credit(data))
		goto out;

#ifndef SPRDWL_RX_NO_WORKQUEUE
	msg = sprdwl_alloc_msg_buf(&p_sprdwl_intf->rx_list);
	if (!msg) {
		pr_err("%s no msgbuf\n", __func__);
		goto out;
	}
	sprdwl_fill_msg(msg, NULL, data, len);
	msg->fifo_id = fifo_id;
	sprdwl_queue_msg_buf(msg, &p_sprdwl_intf->rx_list);
	queue_work(p_sprdwl_intf->rx_queue, &p_sprdwl_intf->rx_work);
#else
	sprdwl_rx_process(data, len, fifo_id);
#endif

	return 0;
out:
	sdiom_pt_read_release(fifo_id);
	return 0;
}

struct sprdwl_intf *sprdwl_intf_create()
{
	struct sprdwl_intf *intf;

	intf = kzalloc(sizeof(*intf), GFP_ATOMIC);
	if (IS_ERR(intf)) {
		pr_err("allocate fail\n");
		return NULL;
	}
	intf->if_ops = &sprdwl_sdio_ops;
	return intf;
}

void sprdwl_intf_free(struct sprdwl_intf *intf)
{
	kfree(intf);
}

void sprdwl_intf_predeinit(struct sprdwl_intf *intf)
{
	intf->driver_status = 0;
	sprdwl_sdio_flush_cp_data(intf->priv);
}

int sprdwl_intf_init(struct sprdwl_intf *intf,
				struct platform_device *pdev,
				struct sprdwl_priv *priv)
{
	int ret;

	ret = start_marlin(MARLIN_WIFI);
	if (ret) {
		pr_err("%s power on chipset failed\n", __func__);
		return -ENODEV;
	}

	intf->pdev = pdev;
	intf->driver_status = 1;
	intf->priv = priv;
	p_sprdwl_intf = intf;

	spin_lock_init(&intf->lock);
	init_waitqueue_head(&intf->waitq);
	atomic_set(&intf->flow0, 0);
	atomic_set(&intf->flow1, 0);
	atomic_set(&intf->flow2, 0);
	intf->cmd_timeout = msecs_to_jiffies(SPRDWL_TX_CMD_TIMEOUT);
	intf->data_timeout = msecs_to_jiffies(SPRDWL_TX_DATA_TIMEOUT);
	intf->wake_timeout = msecs_to_jiffies(SPRDWL_WAKE_TIMEOUT);
	intf->wake_pre_timeout = msecs_to_jiffies(SPRDWL_WAKE_PRE_TIMEOUT);
	intf->wake_last_time = jiffies;
	wake_lock_init(&intf->keep_wake,
		       WAKE_LOCK_SUSPEND, "sprdwl_keep_wakelock");
	ret = sprdwl_msg_init(SPRDWL_RX_MSG_NUM, &intf->tx_list0);
	if (ret) {
		dev_err(&intf->pdev->dev, "%s no tx_list0\n", __func__);
		goto err_tx_list0;
	}
	/* fixme return */
	ret = sprdwl_msg_init(SPRDWL_TX_MSG_DATA_NUM, &intf->tx_list1);
	if (ret) {
		dev_err(&intf->pdev->dev, "%s no tx_list1\n", __func__);
		goto err_tx_list1;
	}

	ret = sprdwl_msg_init(SPRDWL_TX_MSG_DATA_NUM, &intf->tx_list2);
	if (ret) {
		dev_err(&intf->pdev->dev, "%s no tx_list2\n", __func__);
		goto err_tx_list2;
	}

#ifndef SPRDWL_RX_NO_WORKQUEUE
	ret = sprdwl_msg_init(SPRDWL_RX_MSG_NUM, &intf->rx_list);
	if (ret) {
		dev_err(&intf->pdev->dev, "%s no rx_list:%d\n", __func__, ret);
		goto err_rx_list;
	}
#endif

#ifdef SPRDWL_TX_SELF
	ret = sprdwl_tx_buf_init(&intf->txbuf, SPRDWL_SELF_TX_LEN);
	if (ret) {
		dev_err(&intf->pdev->dev, "%s tx_buf create failed", __func__);
		goto err_tx_buf;
	}
	wake_lock_init(&intf->tx_wakelock,
		       WAKE_LOCK_SUSPEND, "sprdwl_tx_wakelock");
#endif
	intf->tx_queue = alloc_ordered_workqueue("SPRDWL_TX_QUEUE",
						 WQ_MEM_RECLAIM |
						 WQ_HIGHPRI | WQ_CPU_INTENSIVE);
	if (!intf->tx_queue) {
		dev_err(&intf->pdev->dev,
			"%s SPRDWL_TX_QUEUE create failed", __func__);
		ret = -ENOMEM;
		/* temp debug */
		goto err_tx_work;
	}
	INIT_WORK(&intf->tx_work, sprdwl_tx_work_queue);

#ifndef SPRDWL_RX_NO_WORKQUEUE
	/* rx init */
	intf->rx_queue = alloc_ordered_workqueue("SPRDWL_RX_QUEUE",
						 WQ_MEM_RECLAIM |
						 WQ_HIGHPRI | WQ_CPU_INTENSIVE);
	if (!intf->rx_queue) {
		dev_err(&intf->pdev->dev, "%s SPRDWL_RX_QUEUE create failed",
			__func__);
		ret = -ENOMEM;
		goto err_rx_work;
	}
	INIT_WORK(&intf->rx_work, sprdwl_rx_work_queue);
#endif

	sdiom_register_pt_tx_release(SPRDWL_SDIOM_WIFI,
				     SPRDWL_SDIOM_CMD_TX,
				     consume_skb);
	sdiom_register_pt_tx_release(SPRDWL_SDIOM_WIFI,
				     SPRDWL_SDIOM_DATA_TX,
				     consume_skb);

	sdiom_register_pt_rx_process(SPRDWL_SDIOM_WIFI,
				     SPRDWL_SDIOM_CMD_RX,
				     sprdwl_sdio_rx_handle);
	sdiom_register_pt_rx_process(SPRDWL_SDIOM_WIFI,
				     SPRDWL_SDIOM_DATA_RX,
				     sprdwl_sdio_rx_handle);
	return 0;

#ifndef SPRDWL_RX_NO_WORKQUEUE
err_rx_work:
	destroy_workqueue(intf->tx_queue);
#endif
#ifndef SPRDWL_TX_SELF
err_tx_work:
#ifndef SPRDWL_RX_NO_WORKQUEUE
	sprdwl_msg_deinit(&intf->rx_list);
#endif
#else
err_tx_work:
	sprdwl_tx_buf_deinit(&intf->txbuf);
	wake_lock_destroy(&intf->tx_wakelock);
err_tx_buf:
#ifndef SPRDWL_RX_NO_WORKQUEUE
	sprdwl_msg_deinit(&intf->rx_list);
#endif
#endif
#ifndef SPRDWL_RX_NO_WORKQUEUE
err_rx_list:
	sprdwl_msg_deinit(&intf->tx_list2);
#endif
err_tx_list2:
	sprdwl_msg_deinit(&intf->tx_list1);
err_tx_list1:
	sprdwl_msg_deinit(&intf->tx_list0);
err_tx_list0:
	wake_lock_destroy(&intf->keep_wake);
	return ret;
}

void sprdwl_intf_deinit(struct sprdwl_intf *intf)
{
	intf->exit = 1;

	sdiom_register_pt_rx_process(SPRDWL_SDIOM_WIFI,
				     SPRDWL_SDIOM_CMD_RX, NULL);
	sdiom_register_pt_rx_process(SPRDWL_SDIOM_WIFI,
				     SPRDWL_SDIOM_DATA_RX, NULL);

	wake_up_all(&intf->waitq);
	flush_workqueue(intf->tx_queue);
	destroy_workqueue(intf->tx_queue);

#ifndef SPRDWL_RX_NO_WORKQUEUE
	flush_workqueue(intf->rx_queue);
	destroy_workqueue(intf->rx_queue);
#endif

	sprdwl_sdio_flush_all_txlist(intf);
#ifdef SPRDWL_TX_SELF
	sprdwl_tx_buf_deinit(&intf->txbuf);
	wake_lock_destroy(&intf->tx_wakelock);
#endif
	wake_lock_destroy(&intf->keep_wake);
	sprdwl_msg_deinit(&intf->tx_list0);
	sprdwl_msg_deinit(&intf->tx_list1);
	sprdwl_msg_deinit(&intf->tx_list2);
#ifndef SPRDWL_RX_NO_WORKQUEUE
	sprdwl_msg_deinit(&intf->rx_list);
#endif
	stop_marlin(MARLIN_WIFI);
	pr_info("%s\t"
		"net: stop %u, start %u\t"
		"drop cnt: cmd %u, sta %u, p2p %u\t"
		"ring_ap:%u ring_cp:%u common:%u sta:%u p2p:%u\n",
		__func__,
		intf->net_stop_cnt, intf->net_start_cnt,
		intf->drop_cmd_cnt, intf->drop_data1_cnt,
		intf->drop_data2_cnt,
		intf->ring_ap, intf->ring_cp,
		atomic_read(&intf->flow0),
		atomic_read(&intf->flow1),
		atomic_read(&intf->flow2));
}

struct sprdwl_sdio_flush_cp {
	u8 num;
	u8 resrv[3];
} __packed;

static int sprdwl_sdio_flush_num(struct sprdwl_priv *priv, int num)
{
	int i, ret, count = 0;
	unsigned short len, cur_len;
	unsigned char *buf;
	struct sprdwl_sdiom_puh *puh;
	struct sprdwl_data_hdr hdr;
	struct wake_lock wake;

	/* CP tx num is less than 80 */
	if (num > 60)
		return -EINVAL;
	/* 2k is enought to flush data msg */
	buf = kmalloc(2048, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	wake_lock_init(&wake,
		       WAKE_LOCK_SUSPEND, "sprdwl_flush_wakelock");
	wake_lock(&wake);
	while (!sdiom_resume_wait_status()) {
		usleep_range(2000, 3000);
		if (count++ >= 1000) {
			dev_err(&p_sprdwl_intf->pdev->dev,
				"%s wait resume erro\n",
				__func__);
			kfree(buf);
			wake_unlock(&wake);
			wake_lock_destroy(&wake);
			return -1;
		}
	}
	marlin_set_sleep(MARLIN_WIFI_FLUSH, FALSE);
	ret = marlin_set_wakeup(MARLIN_WIFI_FLUSH);
	if (ret) {
		dev_err(&p_sprdwl_intf->pdev->dev, "%s sdiom wakeup erro:%d\n",
			__func__, ret);
		goto out;
	}
	/* cp wake code */
	cur_len = 0;
	memset(&hdr, 0, sizeof(hdr));
	hdr.common.type = SPRDWL_TYPE_DATA;
	hdr.common.mode = SPRDWL_MODE_NONE;
	hdr.info1 = SPRDWL_DATA_TYPE_NORMAL;
	hdr.plen = cpu_to_le16(sizeof(hdr));

	len = ALIGN_4BYTE(sizeof(hdr) + sizeof(*puh));

	for (i = 0; i < num; i++) {
		puh = (struct sprdwl_sdiom_puh *)(buf + cur_len);
		puh->type = SPRDWL_SDIOM_WIFI;
		puh->subtype = SPRDWL_SDIOM_DATA_TX;
		puh->len = sizeof(hdr);
		puh->eof = 0;
		puh->pad = 0;
		memcpy(puh + 1, &hdr, sizeof(hdr));
		cur_len += len;
	}
	puh = (struct sprdwl_sdiom_puh *)(buf + cur_len);
	puh->type = 0;
	puh->subtype = 0;
	puh->len = 0;
	puh->eof = 1;
	puh->pad = 0;
	cur_len += sizeof(*puh);
	cur_len = ALIGN_512BYTE(cur_len);

	ret = sdiom_sdio_pt_write_raw(buf, cur_len, 0);
	if (ret < 0)
		dev_err(&p_sprdwl_intf->pdev->dev, "%s sdiom write erro:%d\n",
			__func__, ret);

out:
	kfree(buf);
	marlin_set_sleep(MARLIN_WIFI_FLUSH, TRUE);
	wake_unlock(&wake);
	wake_lock_destroy(&wake);

	return ret;
}

int sprdwl_sdio_flush_cp_data(struct sprdwl_priv *priv)
{
	u16 r_len, s_len;
	int ret;
	struct sprdwl_sdio_flush_cp s_info;
	struct sprdwl_sdio_flush_cp r_info;

	s_len = sizeof(s_info);
	r_len = sizeof(r_info);
	memset(&s_info, 0, sizeof(s_info));
	ret = sprdwl_specify_cmd_send_recv(priv, WIFI_CMD_PRE_CLOSE,
					   SPRDWL_MODE_NONE, (u8 *)&s_info,
					   s_len, (u8 *)&r_info, &r_len);
	if (ret)
		return ret;
	if (r_info.num) {
		dev_info(&p_sprdwl_intf->pdev->dev, "%s flush count:%d\n",
			 __func__, r_info.num);
		ret = sprdwl_sdio_flush_num(priv, r_info.num);
		if (ret)
			dev_err(&p_sprdwl_intf->pdev->dev, "%s flush err:%d\n",
				__func__, ret);
	}

	return ret;
}
