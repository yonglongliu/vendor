/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
//#define DEBUG 0
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include "tee_core.h"
#include "tee_debugfs.h"

static struct dentry *tee_debugfs_dir;

#define LOG_BUF_SIZE    (1024*1024-64)

struct log_buf{
	uint32_t log_magic;
	uint32_t avliable_bytes;
	uint32_t read_pos;
	uint32_t write_pos;
	uint32_t log_no;
	uint32_t reserved[11];
	uint8_t buf[LOG_BUF_SIZE];
};
extern struct log_buf *p_log_buf ;

static ssize_t tee_trace_read(struct file *filp, char __user *userbuf,
			      size_t count, loff_t *ppos)
{
	struct tee *tee = filp->private_data;
	ssize_t ret = 0;

	//printk(KERN_ERR "wait for intel_debugfs!");
	if (down_interruptible(&tee->intel_debugfs))
		printk(KERN_ERR "log buf have some error!");
	//printk(KERN_ERR "got intel_debugfs!");
#if 0
	return simple_read_from_buffer(userbuf, count, ppos, buff, len);
	if(p_log_buf->avliable_bytes > LOG_BUF_SIZE ){
		printk(KERN_ERR "--------------------exception case--------------------");
		ret = copy_to_user( userbuf, p_log_buf->buf + p_log_buf->read_pos, LOG_BUF_SIZE - p_log_buf->read_pos);
		if(ret == count){
			ret = -EFAULT;
		}else{
			ret = copy_to_user( userbuf + LOG_BUF_SIZE - p_log_buf->read_pos , p_log_buf->buf, p_log_buf->write_pos % LOG_BUF_SIZE );
			if(ret == count){
				ret = -EFAULT;
			}else{
				ret = LOG_BUF_SIZE;
			}
		}
	}else if (p_log_buf->avliable_bytes > 0){
		ret = copy_to_user( userbuf, p_log_buf->buf + p_log_buf->read_pos, p_log_buf->avliable_bytes);
		if(ret == count){
			ret = -EFAULT;
		}else{
			ret = p_log_buf->avliable_bytes;
		}
	}else{
		ret = 0;
	}
#else
	ret = copy_to_user( userbuf, p_log_buf , 1024*1024 );
	if(ret == 1024 * 1024) {
		ret = -EFAULT;
	}else{
		ret = 1024*1024;
	}
#endif
	p_log_buf->avliable_bytes = 0;
	p_log_buf->read_pos = 0;
	p_log_buf->write_pos = 0;
	//printk(KERN_ERR "release intel_debugfs_1\n");
	//up(&tee->intel_debugfs_1);
	return ret;

}

static const struct file_operations log_tee_ops = {
	.read = tee_trace_read,
	.open = simple_open,
	.llseek = generic_file_llseek,
};

static uint8_t log_blob_test_buf[1024];
static struct debugfs_blob_wrapper log_blob;

void tee_create_debug_dir(struct tee *tee)
{
	struct dentry *entry;
	struct device *dev = tee->miscdev.this_device;

	if (!tee_debugfs_dir)
		return;

	tee->dbg_dir = debugfs_create_dir(dev_name(dev), tee_debugfs_dir);
	if (!tee->dbg_dir)
		goto error_create_file;

	entry = debugfs_create_file("log", S_IRUGO, tee->dbg_dir,
				    tee, &log_tee_ops);
	if (!entry)
		goto error_create_file;

	sprintf(log_blob_test_buf, "this is blob test message!");
	log_blob.data = (void*)log_blob_test_buf;
	log_blob.size = 1024;
	entry = debugfs_create_blob("log_blob", S_IRUGO, tee->dbg_dir, &log_blob);
	if (!entry)
		goto error_create_file;
	return;

error_create_file:
	dev_err(dev, "can't create debugfs file\n");
	tee_delete_debug_dir(tee);
}

void tee_delete_debug_dir(struct tee *tee)
{
	if (!tee || !tee->dbg_dir)
		return;

	debugfs_remove_recursive(tee->dbg_dir);
}

//void __init tee_init_debugfs(void)
void tee_init_debugfs(void)
{
	if (debugfs_initialized()) {
		tee_debugfs_dir = debugfs_create_dir("tee", NULL);
		if (IS_ERR(tee_debugfs_dir))
			pr_err("can't create debugfs dir\n");
	}
}

//void __exit tee_exit_debugfs(void)
void tee_exit_debugfs(void)
{
	if (tee_debugfs_dir)
		debugfs_remove(tee_debugfs_dir);
}
