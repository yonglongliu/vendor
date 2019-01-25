#ifndef __WDTEESMC_H__
#define __WDTEESMC_H__
/*
 *  * wd tz_driver smc cmd
 *   */
#define TEESMC_RV(func_num) \
	        ((1 << 31) | \
			               ((0) << 30) | \
			               (62 << 24) | \
			               ((func_num) & 0xffff))
#define SMC_CMD_WDTZ_INIT_FINISH  TEESMC_RV(0)
#define SMC_CMD_NORMAL  TEESMC_RV(1)
#define SMC_CMD_SECURE  TEESMC_RV(2)
#define SMC_CMD_IRQ     TEESMC_RV(3)
#define SMC_CMD_IRQ_RET TEESMC_RV(4)

#endif
