#ifndef __STATISTICS_DEBUG_H__
#define __STATISTICS_DEBUG_H__
#include "cfg80211.h"

#define HARD_IRQ_NUM	192
#define MAX_DEAUTH_REASON 256

#define LOCAL_EVENT  0
#define REMOTE_EVENT 1

struct wakeup_rx_count {
	atomic_t wakeup_by_gpio;
	unsigned long cmd_count;
	unsigned long event_count;
	unsigned long data_count;
	unsigned long pcie_addr_count;
};

struct deauth_info {
	spinlock_t lock; /* spinlock for deauth statistics */
	struct {
		unsigned long local_deauth[MAX_DEAUTH_REASON];
		unsigned long remote_deauth[MAX_DEAUTH_REASON];
	} mode[SPRDWL_MODE_MAX];
};

struct intr_info {
	int irq;
	char *irq_name;
};

void sprdwl_pm_suspend_debug(void);
void sprdwl_pm_resume_debug(void);
void check_and_reset_wakeup_flag(void);
void wakeup_reason_statistics(void *rx_data);
void dump_deauth_reason(int mode, u16 reason_code, int dirction);
void stat_debug_info_init(void);
void stat_debug_info_deinit(void);
#endif
