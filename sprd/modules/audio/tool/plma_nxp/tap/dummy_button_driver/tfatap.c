#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>

//#include <asm/irq.h>
//#include <asm/io.h>

#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <asm/param.h>


//timer test
struct timer_list exp_timer;

static void button_interrupt(int irq, void *dummy);

static void do_something(unsigned long data)
{
	static int cnt=0,next=0;

	printk(KERN_INFO "Your timer expired:%d, button=%d\n",cnt++,next);
         mod_timer(&exp_timer, jiffies + 1 *HZ);

         button_interrupt(next++,NULL);
         if(next>4)
			next=0;
}

static int  tst_init(void)
{       
        int delay = 2;
    
        init_timer_on_stack(&exp_timer);

        exp_timer.expires = jiffies + delay * HZ;
        exp_timer.data = 0;
        exp_timer.function = do_something;

        add_timer(&exp_timer);

        return 0;
}

static void  tst_exit(void)
{       
        del_timer(&exp_timer);  

}

/* device structure */
#define MAX_TAP_KEYS 4

struct tfatap {
	struct input_dev *buttons;
	unsigned short keymap[MAX_TAP_KEYS]; /*later ?*/
};
static struct tfatap *tfatap_dev;

static void button_interrupt(int irq, void *dummy)
{
	int btn=BTN_0 + irq;

	input_report_key(tfatap_dev->buttons,btn,1);
	input_report_key(tfatap_dev->buttons,btn,0);
	input_sync(tfatap_dev->buttons);
}

static int __init button_init(void)
{
	int error;
	struct input_dev *idev;

	/* allocate instance struct and input dev */
	tfatap_dev = kzalloc(sizeof *tfatap_dev, GFP_KERNEL);
	idev = input_allocate_device();
	if (!tfatap_dev || !idev) {
		printk(KERN_ERR "button.c: Not enough memory\n");
		error = -ENOMEM;
		return error;
	}

	tfatap_dev->buttons=idev;
	idev->name = "wimtaptest";
	idev->phys = "tfatap-keys/input0";
	idev->id.bustype = BUS_I2C;
	__set_bit(EV_KEY, idev->evbit);
	__set_bit(BTN_0, idev->keybit);
	__set_bit(BTN_1, idev->keybit);
	__set_bit(BTN_2, idev->keybit);
	__set_bit(BTN_3, idev->keybit);
	__set_bit(BTN_4, idev->keybit);

	error = input_register_device(idev);
	if (error) {
		printk(KERN_ERR "tfatap.c: Failed to register device\n");
		goto err_free_dev;
	}

	tst_init();
	
	return 0;

 err_free_dev:
	input_free_device(idev);
	kfree(tfatap_dev);

	return error;
}

static void __exit button_exit(void)
{
	tst_exit();
    input_unregister_device(tfatap_dev->buttons);
    kfree(tfatap_dev);
}

module_init(button_init);
module_exit(button_exit);

