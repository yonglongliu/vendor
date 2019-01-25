/*
 * FM Radio  driver  with SPREADTRUM SC2331FM Radio chip
 *
 * Copyright (c) 2015 Spreadtrum
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/compat.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/miscdevice.h>
#include  <linux/module.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/wakelock.h>

#include "fmdrv.h"
#include "fmdrv_main.h"
#include "fmdrv_ops.h"
#ifndef FM_TEST
#include <linux/marlin_platform.h>
#include <linux/sdiom_rx_api.h>
#include <linux/sdiom_tx_api.h>
#endif
#ifdef CONFIG_OF
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif

struct wake_lock fm_wakelock;

long fm_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
    void __user *argp = (void __user *)arg;
    long ret = 0;
    u32 iarg = 0;
    pr_debug("FM_IOCTL cmd: 0x%x.\n", cmd);
    switch (cmd) {
    case FM_IOCTL_POWERUP:
        fm_powerup(argp);
        ret = fm_tune(argp);
        break;
    case FM_IOCTL_POWERDOWN:
        ret = fm_powerdown();
        break;
    case FM_IOCTL_TUNE:
        ret = fm_tune(argp);
        break;
    case FM_IOCTL_SEEK:
        ret = fm_seek(argp);
        break;
    case FM_IOCTL_SETVOL:
        pr_info("fm ioctl set volume\n");
        ret = fm_set_volume(argp);
        break;
    case FM_IOCTL_GETVOL:
        pr_info("fm ioctl get volume\n");
        ret = fm_get_volume(argp);
        break;
    case FM_IOCTL_MUTE:
        ret = fm_mute(argp);
        break;
    case FM_IOCTL_GETRSSI:
        pr_info("fm ioctl get RSSI\n");
        ret = fm_getrssi(argp);
        break;
    case FM_IOCTL_SCAN:
        pr_info("fm ioctl SCAN\n");
        ret = fm_scan_all(argp);
        break;
    case FM_IOCTL_STOP_SCAN:
        pr_info("fm ioctl STOP SCAN\n");
        ret = fm_stop_scan(argp);
        break;
    case FM_IOCTL_GETCHIPID:
        pr_info("fm ioctl GET chipID\n");
        iarg = 0x2341;
        if (copy_to_user(argp, &iarg, sizeof(iarg)))
            ret = -EFAULT;
        else
            ret = 0;
        break;
    case FM_IOCTL_EM_TEST:
        pr_info("fm ioctl EM_TEST\n");
        ret = 0;
        break;
    case FM_IOCTL_RW_REG:
        pr_info("fm ioctl RW_REG\n");
        ret = fm_rw_reg(argp);
        break;
    case FM_IOCTL_GETMONOSTERO:
        pr_info("fm ioctl GETMONOSTERO\n");
        ret = fm_get_monostero(argp);
        break;
    case FM_IOCTL_GETCURPAMD:
        pr_info("fm ioctl get PAMD\n");
        ret = fm_getcur_pamd(argp);
        break;
	case FM_IOCTL_GETGOODBCNT:
		ret = 0;
		break;

	case FM_IOCTL_GETBADBNT:
		ret = 0;
		break;

	case FM_IOCTL_GETBLERRATIO:
		ret = 0;
		break;

	case FM_IOCTL_RDS_ONOFF:
		pr_info("----RDS_ONOFF----");
        ret = fm_rds_onoff(argp);
        break;
    case FM_IOCTL_RDS_SUPPORT:
        pr_info("fm ioctl is RDS_SUPPORT\n");
        ret = 0;
        if (copy_from_user(&iarg, (void __user *)arg, sizeof(iarg))) {
            pr_err("fm RDS support 's ret value is -eFAULT\n");
            return -EFAULT;
        }
        iarg = FM_RDS_ENABLE;
        if (copy_to_user((void __user *)arg, &iarg, sizeof(iarg)))
            ret = -EFAULT;
        break;
	case FM_IOCTL_RDS_SIM_DATA:
		ret = 0;
		break;

	case FM_IOCTL_IS_FM_POWERED_UP:
		ret = 0;
		break;

	case FM_IOCTL_OVER_BT_ENABLE:
		ret = 0;
		break;

	case FM_IOCTL_ANA_SWITCH:
		ret = fm_ana_switch(argp);
		break;

	case FM_IOCTL_GETCAPARRAY:
		ret = 0;
		break;

	case FM_IOCTL_I2S_SETTING:
		ret = 0;
		break;
	case FM_IOCTL_RDS_GROUPCNT:
		ret = 0;
		break;

	case FM_IOCTL_RDS_GET_LOG:
		ret = 0;
		break;

	case FM_IOCTL_SCAN_GETRSSI:
		ret = 0;
		break;

	case FM_IOCTL_SETMONOSTERO:
		ret = 0;
		break;

	case FM_IOCTL_RDS_BC_RST:
		ret = 0;
		break;

	case FM_IOCTL_CQI_GET:
		ret = 0;
		break;

	case FM_IOCTL_GET_HW_INFO:
		ret = 0;
		break;

	case FM_IOCTL_GET_I2S_INFO:
		ret = 0;
		break;

	case FM_IOCTL_IS_DESE_CHAN:
		ret = 0;
		break;

	case FM_IOCTL_TOP_RDWR:
		ret = 0;
		break;

	case FM_IOCTL_HOST_RDWR:
		ret = 0;
		break;

	case FM_IOCTL_PRE_SEARCH:
		ret = 0;
		break;

	case FM_IOCTL_RESTORE_SEARCH:
		ret = 0;
		break;

	case FM_IOCTL_GET_AUDIO_INFO:
		ret = 0;
		break;

	case FM_IOCTL_SCAN_NEW:
		ret = 0;
		break;

	case FM_IOCTL_SEEK_NEW:
		ret = 0;
		break;

	case FM_IOCTL_TUNE_NEW:
		ret = 0;
		break;

	case FM_IOCTL_SOFT_MUTE_TUNE:
		ret = 0;
		break;

	case FM_IOCTL_DESENSE_CHECK:
		ret = 0;
		break;

	case FM_IOCTL_FULL_CQI_LOG:
		ret = 0;
		break;

	case FM_IOCTL_SET_AUDIO_MODE:
		ret = fm_set_audio_mode(argp);
		break;

	case FM_IOCTL_SET_REGION:
		ret = fm_set_region(argp);
		break;

	case FM_IOCTL_SET_SCAN_STEP:
		ret = fm_set_scan_step(argp);
		break;

	case FM_IOCTL_CONFIG_DEEMPHASIS:
		ret = fm_config_deemphasis(argp);
		break;

	case FM_IOCTL_GET_AUDIO_MODE:
		ret = fm_get_audio_mode(argp);
		break;

	case FM_IOCTL_GET_CUR_BLER:
		ret = fm_get_current_bler(argp);
		break;

	case FM_IOCTL_GET_SNR:
		ret = fm_get_cur_snr(argp);
		break;

	case FM_IOCTL_SOFTMUTE_ONOFF:
		ret = fm_softmute_onoff(argp);
		break;

	case FM_IOCTL_SET_SEEK_CRITERIA:
		ret = fm_set_seek_criteria(argp);
		break;

	case FM_IOCTL_SET_AUDIO_THRESHOLD:
		ret = fm_set_audio_threshold(argp);
		break;

	case FM_IOCTL_GET_SEEK_CRITERIA:
		ret = fm_get_seek_criteria(argp);
		break;

	case FM_IOCTL_GET_AUDIO_THRESHOLD:
		ret = fm_get_audio_threshold(argp);
		break;

	case FM_IOCTL_AF_ONOFF:
		ret = fm_af_onoff(argp);
		break;

	case FM_IOCTL_DUMP_REG:
		ret = 0;
		break;

	default:
		pr_info("Unknown FM IOCTL!\n");
		pr_info("****************: 0x%x.\n", cmd);
		return -EINVAL;
	}

	return ret;
}

int fm_open(struct inode *inode, struct file *filep) {
    pr_info("start open SPRD fm module...\n");
    return 0;
}

int fm_release(struct inode *inode, struct file *filep) {
    pr_info("fm_misc_release.\n");
    fm_powerdown();
    stop_marlin(MARLIN_FM);
    wake_up_interruptible(&fmdev->rds_han.rx_queue);
    fmdev->rds_han.new_data_flag = 1;
    return 0;
}

#ifdef CONFIG_COMPAT
static long fm_compat_ioctl(struct file *file, unsigned int cmd, unsigned long data) {
    pr_debug("start_fm_compat_ioctl FM_IOCTL cmd: 0x%x.\n", cmd);
    cmd = cmd & 0xFFF0FFFF;
    cmd = cmd | 0x00080000;
    pr_debug("fm_compat_ioctl FM_IOCTL cmd: 0x%x.\n", cmd);
    return fm_ioctl(file, cmd, (unsigned long)compat_ptr(data));
}
#endif

const struct file_operations fm_misc_fops = {
    .owner = THIS_MODULE,
    .open = fm_open,
    .read = fm_read_rds_data,
    .unlocked_ioctl = fm_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = fm_compat_ioctl,
#endif
    .release = fm_release,
};

struct miscdevice fm_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = FM_DEV_NAME,
    .fops = &fm_misc_fops,
};

#ifdef CONFIG_OF
static const struct of_device_id  of_match_table_fm[] = {
    { .compatible = "sprd,marlin3_fm", },
    { },
};
MODULE_DEVICE_TABLE(of, of_match_table_fm);
#endif

static int fm_probe(struct platform_device *pdev) {
    int ret = -EINVAL;
    char *ver_str = FM_VERSION;
#ifdef CONFIG_OF
    struct device_node *np;
    np = pdev->dev.of_node;
#endif

    pr_info(" marlin3 FM driver ");
    pr_info(" Version: %s", ver_str);

    ret = misc_register(&fm_misc_device);
    if (ret < 0) {
        pr_info("misc_register failed!");
        return ret;
    }
    pr_info("fm_init success.\n");
    return 0;
}

static int fm_remove(struct platform_device *pdev) {
    pr_info("exit_fm_driver!\n");
    misc_deregister(&fm_misc_device);
    return 0;
}

#ifdef CONFIG_PM_SLEEP
static int fm_suspend(struct device *dev)
{
	return 0;
}

static int fm_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops fm_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(fm_suspend, fm_resume)
};

static struct platform_driver fm_driver = {
	.driver = {
		.name = "sprd-fm",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		 .of_match_table = of_match_ptr(of_match_table_fm),
#endif
		.pm = &fm_pmops,
	},
	.probe = fm_probe,
	.remove = fm_remove,
};

#ifndef CONFIG_OF
struct platform_device fm_device = {
    .name = "sprd-fm",
    .id = -1,
};
#endif

int  fm_device_init_driver(void) {
    int ret;
#ifndef CONFIG_OF
    ret = platform_device_register(&fm_device);
    if (ret) {
        pr_info("fm: platform_device_register failed: %d\n", ret);
        return ret;
    }
#endif
    ret = platform_driver_register(&fm_driver);
    if (ret) {
#ifndef CONFIG_OF
        platform_device_unregister(&fm_device);
#endif
        pr_info("fm: probe failed: %d\n", ret);
    }
    pr_info("fm: probe success: %d\n", ret);
    wake_lock_init(&fm_wakelock, WAKE_LOCK_SUSPEND, "FM_wakelock");
    return ret;
}

void fm_device_exit_driver(void) {
    platform_driver_unregister(&fm_driver);
    wake_lock_destroy(&fm_wakelock);
}
