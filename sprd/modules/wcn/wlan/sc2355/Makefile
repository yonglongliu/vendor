####add cflag######
ccflags-y += -DSC2355_FTR -DSC2355_TX_FILTER -DSC2355_QOS_MAP
ccflags-y += -DIBSS_SUPPORT -DIBSS_RSN_SUPPORT
ccflags-y += -DNAN_SUPPORT
ccflags-y += -DRTT_SUPPORT
ccflags-y += -DSC2355_RX_NO_LOOP
ccflags-y += -DACS_SUPPORT -DSC2355_HW_CSUM
ccflags-y += -DWMMAC_WFA_CERTIFICATION
ccflags-y += -DCOMPAT_SAMPILE_CODE
ccflags-y += -DRND_MAC_SUPPORT
#ccflags-y += -DTCP_ACK_DROP_SUPPORT
#ccflags-y += -DDFS_MASTER
#ccflags-y += -DSC2355_RX_NAPI

#####module name ###
obj-m += sprdwl_ng.o

#######add .o file#####
sprdwl_ng-objs += main.o cfg80211.o txrx.o cmdevt.o npi.o msg.o work.o vendor.o \
				  tcp_ack.o mm.o reorder.o core_sc2355.o tx_msg_sc2355.o rx_msg_sc2355.o \
				  if_sc2355.o qos.o
sprdwl_ng-objs += defrag.o
sprdwl_ng-objs += ibss.o
sprdwl_ng-objs += nan.o
sprdwl_ng-objs += tracer.o
sprdwl_ng-objs += rf_marlin3.o
sprdwl_ng-objs += rtt.o
sprdwl_ng-objs += api_version.o
sprdwl_ng-objs += rnd_mac_addr.o
#sprdwl_ng-objs += 11h.o


modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@
clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@
kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@
