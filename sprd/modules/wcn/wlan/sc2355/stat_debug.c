#include <linux/suspend.h>
#include <linux/workqueue.h>
#include <linux/kallsyms.h>
#include "msg.h"
#include "stat_debug.h"

#define WAKEUP_TIME_EXPIRED 500

static struct wakeup_rx_count rx_wakeup;
static struct deauth_info deauth;

static void detail_event_timer(unsigned long data);
static void deauth_reason_worker(struct work_struct *work);
static void wakeup_reason_worker(struct work_struct *work);
static void detail_worker(struct work_struct *work);

static DEFINE_TIMER(wakeup_timer, detail_event_timer, 0, 0);
static DECLARE_WORK(deauth_worker, deauth_reason_worker);
static DECLARE_WORK(wakeup_worker, wakeup_reason_worker);
static DECLARE_WORK(wakeup_detail_worker, detail_worker);

static void detail_event_timer(unsigned long data)
{
	pr_info("sprdbg: wake up stat timer expired\n");
	atomic_set(&rx_wakeup.wakeup_by_gpio, 0);
}

static struct intr_info irq_info[HARD_IRQ_NUM] = {
	[24] = {.irq_name = "adi"},
	[26] = {.irq_name = "aon_tmr"},
	[26] = {.irq_name = "aon_tmr"},
	[27] = {.irq_name = "aon_syst"},
	[28] = {.irq_name = "mbox_src_ca53"},
	[29] = {.irq_name = "mbox_tar_ca53"},
	[30] = {.irq_name = "i2c"},
	[31] = {.irq_name = "ana"},

	[50] = {.irq_name = "gpio"},
	[51] = {.irq_name = "kpd"},
	[52] = {.irq_name = "eic"},
	[53] = {.irq_name = "ap_tmr0"},
	[54] = {.irq_name = "ap_tmr1"},
	[55] = {.irq_name = "ap_tmr2"},
	[56] = {.irq_name = "ap_tmr3"},
	[57] = {.irq_name = "ap_tmr4"},
	[58] = {.irq_name = "ap_syst"},
	[61] = {.irq_name = "ca53_wtg"},
	[63] = {.irq_name = "thermal"},

	[66] = {.irq_name = "gpu"},
	[92] = {.irq_name = "vbc_audrcd_agcp"},
	[93] = {.irq_name = "vbc_audply_agcp"},

	[164] = {.irq_name = "wtlcp_lte_wdg_set"},
	[165] = {.irq_name = "wtlcp_tg_wdg_rst"},
	[166] = {.irq_name = "agcp_wdg_ret"},
};

static void deauth_reason_worker(struct work_struct *work)
{
	int i;

#define MODE_STA  1
#define MODE_AP   2
#define MODE_GC   5
#define MODE_GO   6
#define MODE_IBSS 7

	pr_info("sprdbg: == START ==\n");
	for (i = 0; i < MAX_DEAUTH_REASON; i++) {
		if (deauth.mode[SPRDWL_MODE_STATION].local_deauth[i] != 0)
			pr_info("sprdbg: [STA] reason[%d](local) %lu times\n",
				i, deauth.mode[MODE_STA].local_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_STATION].remote_deauth[i] != 0)
			pr_info("sprdbg: [STA] reason[%d](remote) %lu times\n",
				i, deauth.mode[MODE_STA].remote_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_AP].local_deauth[i] != 0)
			pr_info("sprdbg: [AP] reason[%d](local) %lu times\n",
				i, deauth.mode[MODE_AP].local_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_AP].remote_deauth[i] != 0)
			pr_info("sprdbg: [AP] reason[%d](remote) %lu times\n",
				i, deauth.mode[MODE_AP].remote_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_P2P_GO].local_deauth[i] != 0)
			pr_info("sprdbg: [GO] reason[%d](local) %lu times\n",
				i, deauth.mode[MODE_GO].local_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_P2P_GO].remote_deauth[i] != 0)
			pr_info("sprdbg: [GO] reason[%d](remote) %lu times\n",
				i, deauth.mode[MODE_GO].remote_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_P2P_CLIENT].local_deauth[i] != 0)
			pr_info("sprdbg: [GC] reason[%d](local) %lu times\n",
				i, deauth.mode[MODE_GC].local_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_P2P_CLIENT].remote_deauth[i] != 0)
			pr_info("sprdbg: [GC] reason[%d](remote) %lu times\n",
				i, deauth.mode[MODE_GC].remote_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_IBSS].local_deauth[i] != 0)
			pr_info("sprdbg: [IBSS] reason[%d](local) %lu times\n",
				i, deauth.mode[MODE_IBSS].local_deauth[i]);

		if (deauth.mode[SPRDWL_MODE_IBSS].remote_deauth[i] != 0)
			pr_info("sprdbg: [IBSS] reason[%d] %lu times\n",
				i, deauth.mode[MODE_IBSS].remote_deauth[i]);
	}

	pr_info("sprdbg: == END ==\n");
}

static void wakeup_reason_worker(struct work_struct *work)
{
	int i;
	unsigned long expired;
	u32 *hard_irqs;

	hard_irqs = (u32 *)kallsyms_lookup_name("sprd_hard_irq");

	if (!hard_irqs) {
		pr_info("failed to get hard irqs\n");
		return;
	}

	for (i = 0; i < HARD_IRQ_NUM; i++) {
		if (irq_info[i].irq_name != NULL)
			if (hard_irqs[i]) {
				pr_info("sprdbg: wake up by %s(%d), times: %d\n",
					irq_info[i].irq_name, i,
					++irq_info[i].irq);

				expired = msecs_to_jiffies(WAKEUP_TIME_EXPIRED);
				if (!strcmp(irq_info[i].irq_name, "gpio")) {
					atomic_set(&rx_wakeup.wakeup_by_gpio,
						   1);
					mod_timer(&wakeup_timer,
						  jiffies + expired);
				}
			}
	}
}

static void detail_worker(struct work_struct *work)
{
	pr_info("sprdbg: wake up by [CMD] %ld times\n",
		rx_wakeup.cmd_count);
	pr_info("sprdbg: wake up by [EVENT] %ld times\n",
		rx_wakeup.event_count);
	pr_info("sprdbg: wake up by [DATA]: %ld times\n",
		rx_wakeup.data_count);
	pr_info("sprdbg: wake up by [PCIE ADDR]: %ld times\n",
		rx_wakeup.pcie_addr_count);
}

void check_and_reset_wakeup_flag(void)
{
	if (atomic_read(&rx_wakeup.wakeup_by_gpio)) {
		atomic_set(&rx_wakeup.wakeup_by_gpio, 0);
		del_timer_sync(&wakeup_timer);
	}
}

void wakeup_reason_statistics(void *rx_data)
{
	int type;

	if (rx_data == NULL)
		return;

	if (!atomic_read(&rx_wakeup.wakeup_by_gpio))
		return;

	atomic_set(&rx_wakeup.wakeup_by_gpio, 0);
	del_timer_sync(&wakeup_timer);
	type = SPRDWL_HEAD_GET_TYPE(rx_data);

	switch (type) {
	case SPRDWL_TYPE_CMD:
		rx_wakeup.cmd_count++;
		break;

	case SPRDWL_TYPE_EVENT:
		rx_wakeup.event_count++;
		break;

	case SPRDWL_TYPE_DATA_SPECIAL:
	case SPRDWL_TYPE_DATA:
		rx_wakeup.data_count++;
		break;

	case SPRDWL_TYPE_DATA_PCIE_ADDR:
		rx_wakeup.pcie_addr_count++;
		break;

	default:
		pr_info("sprdbg: Unknown message type %#x\n", type);
		break;
	}

	schedule_work(&wakeup_detail_worker);
}

void dump_deauth_reason(int mode, u16 reason_code, int dirction)
{
	if (reason_code > MAX_DEAUTH_REASON) {
		pr_info("sprdbg: deauth reason:%d not record\n", reason_code);
		return;
	}

	spin_lock_bh(&deauth.lock);
	switch (dirction) {
	case LOCAL_EVENT:
		deauth.mode[mode].local_deauth[reason_code]++;
		break;
	default:
		deauth.mode[mode].remote_deauth[reason_code]++;
		break;
	}
	spin_unlock_bh(&deauth.lock);

	schedule_work(&deauth_worker);
}

void sprdwl_pm_suspend_debug(void)
{
	del_timer_sync(&wakeup_timer);
	atomic_set(&rx_wakeup.wakeup_by_gpio, 0);
	cancel_work_sync(&deauth_worker);
	cancel_work_sync(&wakeup_detail_worker);
	cancel_work_sync(&wakeup_worker);
}

void sprdwl_pm_resume_debug(void)
{
	schedule_work(&wakeup_worker);
}

void stat_debug_info_init(void)
{
	atomic_set(&rx_wakeup.wakeup_by_gpio, 0);
}

void stat_debug_info_deinit(void)
{
	del_timer_sync(&wakeup_timer);
	cancel_work_sync(&deauth_worker);
	cancel_work_sync(&wakeup_detail_worker);
	cancel_work_sync(&wakeup_worker);
}
