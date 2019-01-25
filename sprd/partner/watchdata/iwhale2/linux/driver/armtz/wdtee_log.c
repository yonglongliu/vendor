#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/anon_inodes.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>


//modify by song.li.pt
#define  get_unused_fd()  get_unused_fd_flags(0)

struct wdlog_dev {
	    struct device *dev;
		struct miscdevice miscdev;
};

struct shm_log {
	int fd_shm;
	size_t size;
};

struct tee_shm_log {
	unsigned long paddr;
	void *vaddr;
};

struct wdlog_dev *log_dev;
struct tee_shm_log *shm = NULL;

static int tee_share_mmap(struct file *filp, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;
	pgprot_t prot;

	if (shm != NULL) {
		printk(KERN_ERR "[%s] %x => %x (+%zu)\n", __func__,
				(unsigned int)shm->
				paddr, (unsigned int)vma->vm_start,
				size);
#if 0
		prot = vma->vm_page_prot;
#else
		prot = pgprot_noncached(vma->vm_page_prot);
#endif

		if (remap_pfn_range(vma, vma->vm_start,
					shm->paddr >> PAGE_SHIFT, size, prot))
			return -EAGAIN;
		BUG_ON(vma->vm_private_data != NULL);
		vma->vm_private_data = (void *)shm->paddr;
	}
	return 0;
}

static int tee_share_release(struct inode *inode, struct file *filp)
{
	if (shm != NULL) {
		kfree(shm);
		shm = NULL;
	}
	return 0;
}

const struct file_operations tee_log_fops = {
	.owner = THIS_MODULE,
	.release = tee_share_release,
	.mmap = tee_share_mmap,
};

int log_open(struct inode *inode, struct file *filp) {
	return 0;
}

long log_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	struct shm_log *u_shm = (struct shm_log*)arg;
	struct shm_log k_shm;
	struct file *file;
	if (copy_from_user(&k_shm, (void *)u_shm, sizeof(struct shm_log))) {
		printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
		return -1;
	}

	k_shm.fd_shm = get_unused_fd();

	if (k_shm.fd_shm < 0) {
		printk(KERN_ERR "> %s, %d get_unused_fd faild\n", __func__, __LINE__);
		return -ENFILE;
	}

	shm = (struct tee_shm_log *)kmalloc(sizeof(struct tee_shm_log), GFP_KERNEL); 
	if (of_machine_is_compatible("sprd,sc9850s-2h10-native")) {
		shm->paddr = (unsigned long)0x99E02000;
	} else if (of_machine_is_compatible("sprd,sc9860g-1h10-3g")) {
		shm->paddr = (unsigned long)0x94C02000;
	} else if (of_machine_is_compatible("sprd,sc9860g-2h10-native")) {
		shm->paddr = (unsigned long)0x94C02000;
	} else if (of_machine_is_compatible("sprd,sc9860g-3h10-native")) {
		shm->paddr = (unsigned long)0x94C02000;
	} else {
		put_unused_fd(k_shm.fd_shm);
		kfree(shm);
		printk(KERN_ERR "> %s, %d ##### unknown Soc type\n", __func__, __LINE__);
		return -1;
	}
	shm->vaddr = ioremap_nocache(shm->paddr, 4096);

	file = anon_inode_getfile("tee_share_fd", &tee_log_fops,
			shm, O_RDWR);
	if (IS_ERR_OR_NULL(file)) {
		kfree(shm);
		put_unused_fd(k_shm.fd_shm);
		printk(KERN_ERR "anon_inode_getfile faild\n");
		return -ENFILE;
	}
	fd_install(k_shm.fd_shm, file);
	put_user(k_shm.fd_shm, &u_shm->fd_shm);

	return 0;
}

int log_release(struct inode *inode, struct file *filp) {
	return 0;
}

struct file_operations hello_fops = {
	.owner =  THIS_MODULE,
	.unlocked_ioctl = log_ioctl,
	.open = log_open,
	.release = log_release,
};

int wdlog_init(struct device *dev) {

	int rc = 0;
	log_dev = devm_kzalloc(dev, sizeof(struct wdlog_dev), GFP_KERNEL);
	if (log_dev == NULL)
		return -ENOMEM;

	log_dev->dev = dev;
	log_dev->miscdev.parent = dev;
	log_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	log_dev->miscdev.name = "wdlog";
	log_dev->miscdev.fops = &hello_fops;

	rc = misc_register(&log_dev->miscdev);
	if (rc != 0) {
		pr_err("misc_register err ret %d\n", rc);
		return rc;
	}

	pr_info("Register misc device \"%s\" (minor=%d)\n", dev_name(log_dev->miscdev.this_device),
			log_dev->miscdev.minor);
	return 0;
}

int wdlog_remove(void)
{
	if (log_dev->miscdev.minor != MISC_DYNAMIC_MINOR) {
		pr_info("Deregister the misc device \"%s\"\n",
				dev_name(log_dev->miscdev.this_device));
		misc_deregister(&log_dev->miscdev);
	}

	devm_kfree(log_dev->dev, log_dev);
	return 0;
}
