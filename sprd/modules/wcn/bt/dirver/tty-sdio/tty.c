/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/vt_kern.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#ifdef CONFIG_OF
#include <linux/of_device.h>
#endif
#include <linux/compat.h>
#include <linux/tty_flip.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include <linux/marlin_platform.h>
#ifndef MTTY_TEST
#include <linux/sdiom_rx_api.h>
#include <linux/sdiom_tx_api.h>
#endif
#include "tty.h"
#include "lpm.h"
#include "rfkill.h"

#include "alignment/sitm.h"

#define MTTY_DEV_MAX_NR     1

#define MTTY_STATE_OPEN     1
#define MTTY_STATE_CLOSE    0

#define lo4bit(x) ((x)&0x0F)
#define hi4bit(x) (((x)>>4)&0x0F)
#define hexformat "0x%x%x,0x%x%x,0x%x%x,0x%x%x"
#define hexformat5 "%x%x %x%x %x%x %x%x %x%x"
#define hexformat6 "%x%x %x%x %x%x %x%x %x%x %x%x"
#define hexformat7 "%x%x %x%x %x%x %x%x %x%x %x%x %x%x"
#define hexformat8 "%x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x"
#define hexformat9 "%x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x"
#define hexformat10 "%x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x"
#define hexformat11 "%x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x"
#define hexformat12 "%x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x %x%x"

#define SDIOM_WR_DIRECT_MOD
#ifdef SDIOM_WR_DIRECT_MOD
#define SDIOM_WR_DIRECT_MOD_ADDR 0x51004000
#endif

struct semaphore sem_id;

struct rx_data {
    unsigned int channel;
    struct mbuf_t *head;
    struct mbuf_t *tail;
    unsigned int num;
    struct list_head entry;
};

struct mtty_device {
    struct mtty_init_data   *pdata;
    struct tty_port *port0;
    struct tty_port *port1;
    struct tty_struct   *tty;
    struct tty_driver   *driver;

    /* mtty state */
    uint32_t    state;
    /*spinlock_t    stat_lock;*/
    /*spinlock_t    rw_lock;*/
    struct mutex    stat_mutex;
    struct mutex    rw_mutex;
    struct list_head        rx_head;
    /*struct tasklet_struct rx_task;*/
    struct work_struct bt_rx_work;
    struct workqueue_struct *bt_rx_workqueue;
};

static struct mtty_device *mtty_dev;
static unsigned int que_task = 1;
static int que_sche = 1;

/*this is for test only*/
#ifdef MTTY_TEST
#define RX_NUM 100
static void *buf_addr;
static char a[RX_NUM] = {1, 2, 3, 4, 5};
static unsigned int (*rx_cb)(void *addr, unsigned int len,
                        unsigned int fifo_id);
static unsigned int (*tx_cb)(void *addr);
static struct timer_list test_timer;
static unsigned int sdiom_register_pt_rx_process(unsigned int type,
                        unsigned int subtype, void *func)
{
    rx_cb = func;
    return 0;
}

static unsigned int sdiom_register_pt_tx_release(unsigned int type,
                        unsigned int subtype, void *func)
{
    tx_cb = func;
    return 0;
}

unsigned int sdiom_pt_write(void *buf, unsigned int len, int type, int subtype)
{
    buf_addr = buf;
    mod_timer(&test_timer, jiffies + msecs_to_jiffies(30));
    return len;
}

unsigned int sdiom_dt_write(unsigned int system_addr, void *buf,
                unsigned int len)
{
    pr_info("%s %d bytes\n", __func__, len);
    mod_timer(&test_timer, jiffies + msecs_to_jiffies(30));
    return len;
}

unsigned int sdiom_pt_read_release(unsigned int fifo_id)
{
    return 0;
}

void timer_cb(unsigned long data)
{
    rx_cb(a, 10, 0);
    mod_timer(&test_timer, jiffies + msecs_to_jiffies(100));
}

void test_init(void)
{
    int i;

    for (i = 0; i < RX_NUM; i++)
        a[i] = '0' + i % 10;
}
#endif

/* static void mtty_rx_task(unsigned long data) */
static void mtty_rx_work_queue(struct work_struct *work)

{
    int i, ret = 0;
    /*struct mtty_device *mtty = (struct mtty_device *)data;*/
    struct mtty_device *mtty;
    struct rx_data *rx = NULL;

    que_task = que_task + 1;
    if (que_task > 65530)
        que_task = 0;
    pr_info("mtty que_task= %d\n", que_task);
    que_sche = que_sche - 1;

    mtty = container_of(work, struct mtty_device, bt_rx_work);
    if (unlikely(!mtty)) {
        pr_err("mtty_rx_task mtty is NULL\n");
        return;
    }

    mutex_lock(&mtty->stat_mutex);
    if (mtty->state == MTTY_STATE_OPEN) {
        mutex_unlock(&mtty->stat_mutex);

        do {
            mutex_lock(&mtty->rw_mutex);
            if (list_empty_careful(&mtty->rx_head)) {
                pr_err("mtty over load queue done\n");
                mutex_unlock(&mtty->rw_mutex);
                break;
            }
            rx = list_first_entry_or_null(&mtty->rx_head,
                        struct rx_data, entry);
            if (!rx) {
                pr_err("mtty over load queue abort\n");
                mutex_unlock(&mtty->rw_mutex);
                break;
            }
            list_del(&rx->entry);
            mutex_unlock(&mtty->rw_mutex);

            pr_err("mtty over load working at channel: %d, len: %d\n",
                        rx->channel, rx->head->len);
            for (i = 0; i < rx->head->len; i++) {
                ret = tty_insert_flip_char(mtty->port0,
                            *(rx->head->buf+i), TTY_NORMAL);
                if (ret != 1) {
                    i--;
                    continue;
                } else {
                    tty_flip_buffer_push(mtty->port0);
                }
            }
            pr_err("mtty over load cut channel: %d\n", rx->channel);
            kfree(rx->head->buf);
            kfree(rx);

        } while (1);
    } else {
        pr_info("mtty status isn't open, status:%d\n", mtty->state);
        mutex_unlock(&mtty->stat_mutex);
    }
}

static int mtty_rx_cb(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
    int ret = 0, len_send;
    int sha, yu, i;
    struct rx_data *rx;
    unsigned char *sdio_buf = NULL;

    sdio_buf = (unsigned char *)head->buf;
    bt_wakeup_host();

    len_send = ((head->buf[2] & 0x7F) << 9) + (head->buf[1] << 1) + (head->buf[0] >> 7);

    printk("%s() channel: %d, num: %d", __func__, chn, num);
    pr_info("%s() ---mtty receive channel= %d, len_send = %d\n", __func__, chn, len_send);

    if ((len_send) <= 4)
        pr_info("%s() count=%d,"hexformat"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]));
    else if ((len_send) == 5)
        pr_info("%s() count=%d,"hexformat5"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]));
    else if ((len_send) == 6)
        pr_info("%s() count=%d,"hexformat6"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]));
    else if ((len_send) == 7)
        pr_info("%s() count=%d,"hexformat7"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]));
    else if ((len_send) == 8)
        pr_info("%s() count=%d,"hexformat8"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]));
    else if ((len_send) == 9)
        pr_info("%s() count=%d,"hexformat9"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]));
    else if ((len_send) == 10)
        pr_info("%s() count=%d,"hexformat10"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]));
    else if ((len_send) == 11)
        pr_info("%s() count=%d,"hexformat11"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10]));
    else if ((len_send) == 12)
        pr_info("%s() count=%d,"hexformat12"\n",
                    __func__, len_send,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 11]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 11]));

    else
    {
        /*pr_info("%s()  count=%d,"hexformat"..."
                    hexformat"\n", __func__, head->len,
                    hi4bit(sdio_buf[4]), lo4bit(sdio_buf[4]),
                    hi4bit(sdio_buf[5]), lo4bit(sdio_buf[5]),
                    hi4bit(sdio_buf[6]), lo4bit(sdio_buf[6]),
                    hi4bit(sdio_buf[7]), lo4bit(sdio_buf[7]),
                    hi4bit(sdio_buf[(head->len)+BT_SDIO_HEAD_LEN-4]),
                    lo4bit(sdio_buf[(head->len)+BT_SDIO_HEAD_LEN-4]),
                    hi4bit(sdio_buf[(head->len)+BT_SDIO_HEAD_LEN-3]),
                    lo4bit(sdio_buf[(head->len)+BT_SDIO_HEAD_LEN-3]),
                    hi4bit(sdio_buf[(head->len)+BT_SDIO_HEAD_LEN-2]),
                    lo4bit(sdio_buf[(head->len)+BT_SDIO_HEAD_LEN-2]),
                    hi4bit(sdio_buf[(head->len)+BT_SDIO_HEAD_LEN-1]),
                    lo4bit(sdio_buf[(head->len)+BT_SDIO_HEAD_LEN-1]));*/

                    sha = (len_send) / 12;
                    yu = (len_send) % 12;
                    pr_info("mtty_rx_cb() count=%d sha=%d, yu=%d\n", len_send, sha, yu);

                    for (i = 0; i < sha; i++)
                    {
                        pr_info("mtty_rx_cb() "hexformat12"\n",
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10 + i*12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 11 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 11 + i*12]));
                    }

                    if (yu != 0)
                    {
                        pr_info("mtty_rx_cb() 22yu "hexformat12"\n",
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-12]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-12]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-11]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-11]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-10]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-10]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-9]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-9]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-8]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-8]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-7]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-7]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-6]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-6]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-5]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-5]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-4]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-4]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-3]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-3]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-2]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-2]),
                            hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-1]),
                            lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + (len_send)-1]));
                    }

    }

    if (mtty_dev->state != MTTY_STATE_OPEN) {
        pr_err("%s() mtty bt is closed abnormally\n", __func__);

        sprdwcn_bus_push_list(chn, head, tail, num);
        return -1;
    }

    if (mtty_dev != NULL) {
        if (!work_pending(&mtty_dev->bt_rx_work)) {
            pr_info("%s() tty_insert_flip_string", __func__);
            ret = tty_insert_flip_string(mtty_dev->port0,
                            (unsigned char *)head->buf+BT_SDIO_HEAD_LEN,
                            len_send);   // -BT_SDIO_HEAD_LEN
            pr_info("%s() ret=%d, len=%d\n", __func__, ret, len_send);
            if (ret)
                tty_flip_buffer_push(mtty_dev->port0);
            if (ret == (len_send)) {
                pr_info("%s() send success", __func__);
                sprdwcn_bus_push_list(chn, head, tail, num);
                return 0;
            }
        }

        rx = kmalloc(sizeof(struct rx_data), GFP_KERNEL);
        if (rx == NULL) {
            pr_err("%s() rx == NULL\n", __func__);
            sprdwcn_bus_push_list(chn, head, tail, num);
            return -ENOMEM;
        }

        rx->head = head;
        rx->tail = tail;
        rx->channel = chn;
        rx->num = num;
        rx->head->len = (len_send) - ret;
        rx->head->buf = kmalloc(rx->head->len, GFP_KERNEL);
        if (rx->head->buf == NULL) {
            pr_err("mtty low memory!\n");
            kfree(rx);
            sprdwcn_bus_push_list(chn, head, tail, num);
            return -ENOMEM;
        }

        memcpy(rx->head->buf, head->buf, rx->head->len);
        sprdwcn_bus_push_list(chn, head, tail, num);
        mutex_lock(&mtty_dev->rw_mutex);
        pr_err("mtty over load push %d -> %d, channel: %d len: %d\n",
                len_send, ret, rx->channel, rx->head->len);
        list_add_tail(&rx->entry, &mtty_dev->rx_head);
        mutex_unlock(&mtty_dev->rw_mutex);
        if (!work_pending(&mtty_dev->bt_rx_work)) {
        pr_err("work_pending\n");
            queue_work(mtty_dev->bt_rx_workqueue,
                        &mtty_dev->bt_rx_work);
        }
        return 0;
    }
    pr_err("mtty_rx_cb mtty_dev is NULL!!!\n");

    return -1;
}

static int mtty_tx_cb(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
    int i;
    struct mbuf_t *pos = NULL;
    pr_info("%s channel: %d, head: %p, tail: %p num: %d\n", __func__, chn, head, tail, num);
    //if (head->buf)
        //for (i = 0; i < head->len + BT_SDIO_HEAD_LEN; i++)
            //pr_info("%s i%d 0x%x\n", __func__, i, head->buf[i]);
    pos = head;
    for (i = 0; i < num; i++, pos = pos->next) {
        kfree(pos->buf);
        pos->buf = NULL;
    }
    if ((sprdwcn_bus_list_free(chn, head, tail, num)) == 0)
    {
        pr_info("%s sprdwcn_bus_list_free() success\n", __func__);
        up(&sem_id);
    }
    else
        pr_err("%s sprdwcn_bus_list_free() fail\n", __func__);

    return 0;
}

static int mtty_open(struct tty_struct *tty, struct file *filp)
{
    struct mtty_device *mtty = NULL;
    struct tty_driver *driver = NULL;

    if (tty == NULL) {
        pr_err("mtty open input tty is NULL!\n");
        return -ENOMEM;
    }
    driver = tty->driver;
    mtty = (struct mtty_device *)driver->driver_state;

    if (mtty == NULL) {
        pr_err("mtty open input mtty NULL!\n");
        return -ENOMEM;
    }

    mtty->tty = tty;
    tty->driver_data = (void *)mtty;

    mutex_lock(&mtty->stat_mutex);
    mtty->state = MTTY_STATE_OPEN;
    mutex_unlock(&mtty->stat_mutex);
    que_task = 0;
    que_sche = 0;
    sitm_ini();
    pr_info("mtty_open device success!\n");

    return 0;
}

static void mtty_close(struct tty_struct *tty, struct file *filp)
{
    struct mtty_device *mtty = NULL;

    if (tty == NULL) {
        pr_err("mtty close input tty is NULL!\n");
        return;
    }
    mtty = (struct mtty_device *) tty->driver_data;
    if (mtty == NULL) {
        pr_err("mtty close s tty is NULL!\n");
        return;
    }

    mutex_lock(&mtty->stat_mutex);
    mtty->state = MTTY_STATE_CLOSE;
    mutex_unlock(&mtty->stat_mutex);
    sitm_cleanup();
    pr_info("mtty_close device success !\n");
}

static int mtty_write(struct tty_struct *tty,
            const unsigned char *buf, int count)
{
    int num = 1, sha, yu, i;
    struct mbuf_t *tx_head = NULL;
    struct mbuf_t *tx_tail = NULL;

    unsigned char *sdio_buf = NULL;
    pr_info("channel %s\n", __func__);
#if 0
    if (unlikely(marlin_get_download_status() != true))
    {
        pr_err("grandlog mtty_write()1 status = false\n");
        return -EIO;
    }
#endif

    sdio_buf = kmalloc(count + BT_SDIO_HEAD_LEN, GFP_KERNEL);

    if (sdio_buf == NULL)
    {
        pr_err("mtty_write() sdio_buf = null\n");
        return -ENOMEM;
    }

    memset(sdio_buf, 0, count + BT_SDIO_HEAD_LEN);
    memcpy(sdio_buf + BT_SDIO_HEAD_LEN, buf, count);

    if (count <= 4)
        pr_info("mtty_write() count=%d,"hexformat"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]));
    else if (count == 5)
        pr_info("mtty_write() count=%d,"hexformat5"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]));
    else if (count == 6)
        pr_info("mtty_write() count=%d,"hexformat6"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]));
    else if (count == 7)
        pr_info("mtty_write() count=%d,"hexformat7"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]));
    else if (count == 8)
        pr_info("mtty_write() count=%d,"hexformat8"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]));
    else if (count == 9)
        pr_info("mtty_write() count=%d,"hexformat9"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]));
    else if (count == 10)
        pr_info("mtty_write() count=%d,"hexformat10"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]));
    else if (count == 11)
        pr_info("mtty_write() count=%d,"hexformat11"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10]));
    else if (count == 12)
        pr_info("mtty_write() count=%d,"hexformat12"\n",
                    count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 11]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 11]));
    else
    {
        /*pr_info("mtty_write() mtty packer mode write count=%d,"hexformat"..."
                    hexformat"\n", count,
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-4]),
                    lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-4]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-3]),
                    lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-3]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-2]),
                    lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-2]),
                    hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-1]),
                    lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-1]));*/

        sha = count / 12;
        yu = count % 12;
        pr_info("mtty_write() count=%d sha=%d, yu=%d\n", count, sha, yu);

        for (i = 0; i < sha; i++)
        {
            pr_info("mtty_write() "hexformat12"\n",
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 1 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 2 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 3 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 4 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 5 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 6 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 7 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 8 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 9 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 10 + i*12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + 11 + i*12]), lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + 11 + i*12]));
        }

        if (yu != 0)
        {
            pr_info("mtty_write() 222yu "hexformat12"\n",
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-12]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-12]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-11]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-11]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-10]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-10]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-9]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-9]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-8]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-8]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-7]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-7]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-6]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-6]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-5]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-5]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-4]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-4]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-3]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-3]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-2]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-2]),
                        hi4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-1]),
                        lo4bit(sdio_buf[BT_SDIO_HEAD_LEN + count-1]));
        }
    }

    down(&sem_id);
    if (!sprdwcn_bus_list_alloc(BT_TX_CHANNEL0, &tx_head, &tx_tail, &num)) {
        pr_info("%s() sprdwcn_bus_list_alloc() success\n", __func__);
        tx_head->buf = sdio_buf;
        tx_head->len = count;
        tx_head->next = NULL;
        /*packer type 0, subtype 0*/
        pr_info("%s() sprdwcn_bus_push_list() num=%d\n", __func__, num);
        if (sprdwcn_bus_push_list(BT_TX_CHANNEL0, tx_head, tx_tail, num))
        {
            pr_err("mtty write PT data to sdiom fail Error, free buf\n");
            kfree(tx_head->buf);
            tx_head->buf = NULL;
            sprdwcn_bus_list_free(BT_TX_CHANNEL0, tx_head, tx_tail, num);
            return -EBUSY;
        }
        else
        {
            pr_info("%s() sprdwcn_bus_push_list() success\n", __func__);
            return count;
        }
    } else {
        pr_err("%s:%d sprdwcn_bus_list_alloc fail\n", __func__, __LINE__);
        up(&sem_id);
        return -ENOMEM;
    }
}


static  int sdio_data_transmit(uint8_t *data, size_t count)
{
	return mtty_write(NULL, data, count);
}


static int mtty_write_plus(struct tty_struct *tty,
	      const unsigned char *buf, int count)
{
	return sitm_write(buf, count, sdio_data_transmit);
}


static void mtty_flush_chars(struct tty_struct *tty)
{
}

static int mtty_write_room(struct tty_struct *tty)
{
	return INT_MAX;
}

static const struct tty_operations mtty_ops = {
    .open  = mtty_open,
    .close = mtty_close,
    .write = mtty_write_plus,
    .flush_chars = mtty_flush_chars,
    .write_room  = mtty_write_room,
};

static struct tty_port *mtty_port_init(void)
{
    struct tty_port *port = NULL;

    port = kzalloc(sizeof(struct tty_port), GFP_KERNEL);
    if (port == NULL)
        return NULL;
    tty_port_init(port);

    return port;
}

static int mtty_tty_driver_init(struct mtty_device *device)
{
    struct tty_driver *driver;
    int ret = 0;

    device->port0 = mtty_port_init();
    if (!device->port0)
        return -ENOMEM;

    device->port1 = mtty_port_init();
    if (!device->port1)
        return -ENOMEM;

    driver = alloc_tty_driver(MTTY_DEV_MAX_NR * 2);
    if (!driver)
        return -ENOMEM;

    /*
    * Initialize the tty_driver structure
    * Entries in mtty_driver that are NOT initialized:
    * proc_entry, set_termios, flush_buffer, set_ldisc, write_proc
    */
    driver->owner = THIS_MODULE;
    driver->driver_name = device->pdata->name;
    driver->name = device->pdata->name;
    driver->major = 0;
    driver->type = TTY_DRIVER_TYPE_SYSTEM;
    driver->subtype = SYSTEM_TYPE_TTY;
    driver->init_termios = tty_std_termios;
    driver->driver_state = (void *)device;
    device->driver = driver;
    device->driver->flags = TTY_DRIVER_REAL_RAW;
    /* initialize the tty driver */
    tty_set_operations(driver, &mtty_ops);
    tty_port_link_device(device->port0, driver, 0);
    tty_port_link_device(device->port1, driver, 1);
    ret = tty_register_driver(driver);
    if (ret) {
        put_tty_driver(driver);
        tty_port_destroy(device->port0);
        tty_port_destroy(device->port1);
        return ret;
    }
    return ret;
}

static void mtty_tty_driver_exit(struct mtty_device *device)
{
    struct tty_driver *driver = device->driver;

    tty_unregister_driver(driver);
    put_tty_driver(driver);
    tty_port_destroy(device->port0);
    tty_port_destroy(device->port1);
}

static int mtty_parse_dt(struct mtty_init_data **init, struct device *dev)
{
#ifdef CONFIG_OF
    struct device_node *np = dev->of_node;
    struct mtty_init_data *pdata = NULL;
    int ret;

    pdata = kzalloc(sizeof(struct mtty_init_data), GFP_KERNEL);
    if (!pdata)
        return -ENOMEM;

    ret = of_property_read_string(np,
                    "sprd,name",
                    (const char **)&pdata->name);
    if (ret)
        goto error;
    *init = pdata;

    return 0;
error:
    kfree(pdata);
    *init = NULL;
    return ret;
#else
    return -ENODEV;
#endif
}

static inline void mtty_destroy_pdata(struct mtty_init_data **init)
{
#ifdef CONFIG_OF
    struct mtty_init_data *pdata = *init;

    kfree(pdata);

    *init = NULL;
#else
    return;
#endif
}

struct mchn_ops_t bt_rx_ops = {
    .channel = BT_RX_CHANNEL,
    .hif_type = HW_TYPE_SDIO,
    .inout = BT_RX_INOUT,
    .pool_size = BT_RX_POOL_SIZE,
    .pop_link = mtty_rx_cb,
};

struct mchn_ops_t bt_tx_ops0 = {
    .channel = BT_TX_CHANNEL0,
    .hif_type = HW_TYPE_SDIO,
    .inout = BT_TX_INOUT,
    .pool_size = BT_TX_POOL_SIZE0,
    .pop_link = mtty_tx_cb,
};

struct mchn_ops_t bt_tx_ops1 = {
    .channel = BT_TX_CHANNEL1,
    .hif_type = HW_TYPE_SDIO,
    .inout = BT_TX_INOUT,
    .pool_size = BT_TX_POOL_SIZE1,
    .pop_link = mtty_tx_cb,
};

static int  mtty_probe(struct platform_device *pdev)
{
    struct mtty_init_data *pdata = (struct mtty_init_data *)
                                pdev->dev.platform_data;
    struct mtty_device *mtty;
    int rval = 0;

    if (pdev->dev.of_node && !pdata) {
        rval = mtty_parse_dt(&pdata, &pdev->dev);
        if (rval) {
            pr_err("failed to parse mtty device tree, ret=%d\n",
                    rval);
            return rval;
        }
    }

    mtty = kzalloc(sizeof(struct mtty_device), GFP_KERNEL);
    if (mtty == NULL) {
        mtty_destroy_pdata(&pdata);
        pr_err("mtty Failed to allocate device!\n");
        return -ENOMEM;
    }

    mtty->pdata = pdata;
    rval = mtty_tty_driver_init(mtty);
    if (rval) {
        mtty_tty_driver_exit(mtty);
        kfree(mtty->port0);
        kfree(mtty->port1);
        kfree(mtty);
        mtty_destroy_pdata(&pdata);
        pr_err("regitster notifier failed (%d)\n", rval);
        return rval;
    }

    pr_info("mtty_probe init device addr: 0x%p\n", mtty);
    platform_set_drvdata(pdev, mtty);

    /*spin_lock_init(&mtty->stat_lock);*/
    /*spin_lock_init(&mtty->rw_lock);*/
    mutex_init(&mtty->stat_mutex);
    mutex_init(&mtty->rw_mutex);
    INIT_LIST_HEAD(&mtty->rx_head);
    /*tasklet_init(&mtty->rx_task, mtty_rx_task, (unsigned long)mtty);*/
    mtty->bt_rx_workqueue =
        create_singlethread_workqueue("SPRDBT_RX_QUEUE");
    if (!mtty->bt_rx_workqueue) {
        pr_err("%s SPRDBT_RX_QUEUE create failed", __func__);
        return -ENOMEM;
    }
    INIT_WORK(&mtty->bt_rx_work, mtty_rx_work_queue);

    mtty_dev = mtty;

    rfkill_bluetooth_init(pdev);
    bluesleep_init();

    sprdwcn_bus_chn_init(&bt_rx_ops);
    sprdwcn_bus_chn_init(&bt_tx_ops0);
    sprdwcn_bus_chn_init(&bt_tx_ops1);
    sema_init(&sem_id, BT_TX_POOL_SIZE0 - 1);

#ifdef MTTY_TEST
    setup_timer(&test_timer, timer_cb, 0);
    test_init();
#endif
    return 0;
}

static int  mtty_remove(struct platform_device *pdev)
{
    struct mtty_device *mtty = platform_get_drvdata(pdev);

    mtty_tty_driver_exit(mtty);
    sprdwcn_bus_chn_deinit(&bt_rx_ops);
    sprdwcn_bus_chn_deinit(&bt_tx_ops0);
    sprdwcn_bus_chn_deinit(&bt_tx_ops1);
    kfree(mtty->port0);
    kfree(mtty->port1);
    mtty_destroy_pdata(&mtty->pdata);
    flush_workqueue(mtty->bt_rx_workqueue);
    destroy_workqueue(mtty->bt_rx_workqueue);
    /*tasklet_kill(&mtty->rx_task);*/
    kfree(mtty);
    platform_set_drvdata(pdev, NULL);
    bluesleep_exit();

    return 0;
}

static const struct of_device_id mtty_match_table[] = {
    { .compatible = "sprd,mtty", },
    { },
};

static struct platform_driver mtty_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "mtty",
        .of_match_table = mtty_match_table,
    },
    .probe = mtty_probe,
    .remove = mtty_remove,
};

module_platform_driver(mtty_driver);

MODULE_AUTHOR("Spreadtrum BSP");
MODULE_DESCRIPTION("SPRD marlin tty driver");
