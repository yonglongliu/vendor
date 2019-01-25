/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors	:
 * Keguang Zhang <keguang.zhang@spreadtrum.com>
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

#include "sprdwl.h"
#include "npi.h"
#include "cfg80211.h"
#include "cmdevt.h"
#include "txrx.h"
#include "msg.h"
#include "intf_ops.h"
#include "vendor.h"
#include "work.h"
#if defined(SC2355_FTR)
#include "tx_msg_sc2355.h"
#include "rx_msg_sc2355.h"
#include "core_sc2355.h"
#endif
#include "tcp_ack.h"
#ifdef DFS_MASTER
#include "11h.h"
#endif

#ifdef RTT_SUPPORT
#include "rtt.h"
#endif /* RTT_SUPPORT */

struct sprdwl_priv *g_sprdwl_priv;

static void str2mac(const char *mac_addr, u8 *mac)
{
	unsigned int m[ETH_ALEN];

	if (sscanf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) != ETH_ALEN)
		wl_err("failed to parse mac address '%s'", mac_addr);
	mac[0] = m[0];
	mac[1] = m[1];
	mac[2] = m[2];
	mac[3] = m[3];
	mac[4] = m[4];
	mac[5] = m[5];
}

void sprdwl_netif_rx(struct sk_buff *skb, struct net_device *ndev)
{
	struct sprdwl_vif *vif;
	struct sprdwl_intf *intf;
	struct sprdwl_rx_if *rx_if = NULL;

	vif = netdev_priv(ndev);
	intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
	rx_if = (struct sprdwl_rx_if *)intf->sprdwl_rx;

	print_hex_dump_debug("RX packet: ", DUMP_PREFIX_OFFSET,
			     16, 1, skb->data, skb->len, 0);
	skb->dev = ndev;
	skb->protocol = eth_type_trans(skb, ndev);
	/* CHECKSUM_UNNECESSARY not supported by our hardware */
	/* skb->ip_summed = CHECKSUM_UNNECESSARY; */

	ndev->stats.rx_packets++;
	ndev->stats.rx_bytes += skb->len;
#if defined(MORE_DEBUG)
		intf->stats.rx_packets++;
		intf->stats.rx_bytes += skb->len;
		if (skb->pkt_type == PACKET_MULTICAST)
			intf->stats.rx_multicast++;
#endif

#ifndef SC2355_RX_NAPI
	netif_rx(skb);
#else
	skb_orphan(skb);
	napi_gro_receive(&rx_if->napi_rx, skb);
#endif
}

void sprdwl_stop_net(struct sprdwl_vif *vif)
{
	struct sprdwl_vif *real_vif, *tmp_vif;
	struct sprdwl_priv *priv = vif->priv;

	spin_lock_bh(&priv->list_lock);
	list_for_each_entry_safe(real_vif, tmp_vif, &priv->vif_list, vif_node)
		if (real_vif->ndev)
			netif_stop_queue(real_vif->ndev);
	spin_unlock_bh(&priv->list_lock);
}

static void sprdwl_netflowcontrl_mode(struct sprdwl_priv *priv,
				      enum sprdwl_mode mode, bool state)
{
	struct sprdwl_vif *vif;

	vif = mode_to_vif(priv, mode);
	if (vif) {
		if (state)
			netif_wake_queue(vif->ndev);
		else
			netif_stop_queue(vif->ndev);
		sprdwl_put_vif(vif);
	}
}

static void sprdwl_netflowcontrl_all(struct sprdwl_priv *priv, bool state)
{
	struct sprdwl_vif *real_vif, *tmp_vif;

	spin_lock_bh(&priv->list_lock);
	list_for_each_entry_safe(real_vif, tmp_vif, &priv->vif_list, vif_node)
		if (real_vif->ndev) {
			if (state)
				netif_wake_queue(real_vif->ndev);
			else
				netif_stop_queue(real_vif->ndev);
		}
	spin_unlock_bh(&priv->list_lock);
}

/* @state: true for netif_start_queue
 *	   false for netif_stop_queue
 */
void sprdwl_net_flowcontrl(struct sprdwl_priv *priv,
			   enum sprdwl_mode mode, bool state)
{
	if (mode != SPRDWL_MODE_NONE)
		sprdwl_netflowcontrl_mode(priv, mode, state);
	else
		sprdwl_netflowcontrl_all(priv, state);
}

static int sprdwl_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	int ret = 0;
	bool check_tcpack = false;
	struct sprdwl_msg_buf *msg = NULL;

	/* FIXME vif connect state, need fix cfg80211_connect_result when MCC */
	/*if (vif->connect_status != SPRDWL_CONNECTED) */

	/* Hardware tx data queue prority is lower than management queue
	 * management frame will be send out early even that get into queue
	 * after data frame.
	 * Workaround way: Put eap failure frame to high queue
	 * by use tx mgmt cmd
	 */
	/*send 802.1X or WAI frame from cmd channel*/
	if (skb->protocol == cpu_to_be16(ETH_P_PAE) ||
		skb->protocol == cpu_to_be16(WAPI_TYPE)) {
			pr_err("send %s frame by WIFI_CMD_TX_DATA\n",
			       skb->protocol == cpu_to_be16(ETH_P_PAE) ?
			       "802.1X" : "WAI");
			sprdwl_xmit_data2cmd(skb, ndev);
			return NETDEV_TX_OK;
	} else {
		ret = sprdwl_tx_filter_packet(skb, ndev);
		if (!ret)
			return NETDEV_TX_OK;
	}

	/*mode not open, so we will not send data*/
	if (vif->priv->fw_stat[vif->mode] != SPRDWL_INTF_OPEN) {
		printk_ratelimited("%s, %d, error! should not send this data\n",
		       __func__, __LINE__);
		return NETDEV_TX_BUSY;
	}

	msg = sprdwl_intf_get_msg_buf(vif->priv,
				      SPRDWL_TYPE_DATA,
				      vif->mode,
				      vif->ctx_id);
	if (!msg) {
		if (vif->priv->hw_type == SPRDWL_HW_SDIO_BA)
			sprdwl_stop_net(vif);
		ndev->stats.tx_fifo_errors++;
		return NETDEV_TX_BUSY;
	}

	if (skb_headroom(skb) < ndev->needed_headroom) {
		struct sk_buff *tmp_skb = skb;

		skb = skb_realloc_headroom(skb, ndev->needed_headroom);
		dev_kfree_skb(tmp_skb);
		if (!skb) {
			netdev_err(ndev,
				   "%s skb_realloc_headroom failed\n",
				   __func__);
			sprdwl_intf_free_msg_buf(vif->priv, msg);
			goto out;
		}
	}

#if !defined(SC2355_FTR)
	/* sprdwl_send_data: offset use 2 for cp bytes align */
	ret = sprdwl_send_data(vif, msg, skb, 2, check_tcpack);
#else
	ret = sprdwl_send_data(vif, msg, skb, 0, check_tcpack);
#endif /* SC2355_FTR */
	if (ret) {
		netdev_err(ndev, "%s drop msg due to TX Err\n", __func__);
		/* FIXME as debug sdiom later, here just drop the msg
		 * wapi temp drop
		 */
		dev_kfree_skb(skb);
		sprdwl_intf_free_msg_buf(vif->priv, msg);
		return NETDEV_TX_OK;
	}

	vif->ndev->stats.tx_bytes += skb->len;
	vif->ndev->stats.tx_packets++;
	ndev->trans_start = jiffies;
	print_hex_dump_debug("TX packet: ", DUMP_PREFIX_OFFSET,
			     16, 1, skb->data, skb->len, 0);
out:
	return NETDEV_TX_OK;
}

static int sprdwl_init(struct net_device *ndev)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	/* initialize firmware */
	return sprdwl_init_fw(vif);
}

static void sprdwl_uninit(struct net_device *ndev)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	sprdwl_uninit_fw(vif);
}

static int sprdwl_open(struct net_device *ndev)
{
	netdev_info(ndev, "%s\n", __func__);
#ifdef DFS_MASTER
	netif_carrier_off(ndev);
#endif

	netif_start_queue(ndev);

	return 0;
}

static int sprdwl_close(struct net_device *ndev)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	netdev_info(ndev, "%s\n", __func__);

	sprdwl_scan_done(vif, true);
	sprdwl_sched_scan_done(vif, true);
	netif_stop_queue(ndev);
	if (netif_carrier_ok(ndev))
		netif_carrier_off(ndev);

	return 0;
}

static struct net_device_stats *sprdwl_get_stats(struct net_device *ndev)
{
	return &ndev->stats;
}

static void sprdwl_tx_timeout(struct net_device *ndev)
{
	netdev_info(ndev, "%s\n", __func__);

	ndev->trans_start = jiffies;
	netif_wake_queue(ndev);
}

#define CMD_BLACKLIST_ENABLE		"BLOCK"
#define CMD_BLACKLIST_DISABLE		"UNBLOCK"
#define CMD_ADD_WHITELIST		"WHITE_ADD"
#define CMD_DEL_WHITELIST		"WHITE_DEL"
#define CMD_ENABLE_WHITELIST		"WHITE_EN"
#define CMD_DISABLE_WHITELIST		"WHITE_DIS"
#define CMD_SETSUSPENDMODE		"SETSUSPENDMODE"
#define CMD_SET_FCC_CHANNEL		"SET_FCC_CHANNEL"
#define CMD_SET_COUNTRY			"COUNTRY"
#define CMD_11V_GET_CFG			"11VCFG_GET"
#define CMD_11V_SET_CFG			"11VCFG_SET"
#define CMD_11V_WNM_SLEEP		"WNM_SLEEP"
#define CMD_SET_MAX_CLIENTS		"MAX_STA"

static int sprdwl_priv_cmd(struct net_device *ndev, struct ifreq *ifr)
{
	int n_clients;
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_priv *priv = vif->priv;
	struct android_wifi_priv_cmd priv_cmd;
	char *command = NULL, *country = NULL;
	u16 interval = 0;
	u8 feat = 0, status = 0;
	u8 addr[ETH_ALEN] = {0}, *mac_addr = NULL, *tmp, *mac_list;
	int ret = 0, skip, counter, index;

	if (!ifr->ifr_data)
		return -EINVAL;
	if (copy_from_user(&priv_cmd, ifr->ifr_data, sizeof(priv_cmd)))
		return -EFAULT;

	command = kmalloc(priv_cmd.total_len, GFP_KERNEL);
	if (!command)
		return -ENOMEM;
	if (copy_from_user(command, priv_cmd.buf, priv_cmd.total_len)) {
		ret = -EFAULT;
		goto out;
	}

	if (!strncasecmp(command, CMD_BLACKLIST_ENABLE,
			 strlen(CMD_BLACKLIST_ENABLE))) {
		skip = strlen(CMD_BLACKLIST_ENABLE) + 1;
		str2mac(command + skip, addr);
		if (!is_valid_ether_addr(addr))
			goto out;
		netdev_info(ndev, "%s: block %pM\n", __func__, addr);
		ret = sprdwl_set_blacklist(priv, vif->ctx_id,
					   SPRDWL_SUBCMD_ADD, 1, addr);
	} else if (!strncasecmp(command, CMD_BLACKLIST_DISABLE,
				strlen(CMD_BLACKLIST_DISABLE))) {
		skip = strlen(CMD_BLACKLIST_DISABLE) + 1;
		str2mac(command + skip, addr);
		if (!is_valid_ether_addr(addr))
			goto out;
		netdev_info(ndev, "%s: unblock %pM\n", __func__, addr);
		ret = sprdwl_set_blacklist(priv, vif->ctx_id,
					   SPRDWL_SUBCMD_DEL, 1, addr);
	} else if (!strncasecmp(command, CMD_ADD_WHITELIST,
				strlen(CMD_ADD_WHITELIST))) {
		skip = strlen(CMD_ADD_WHITELIST) + 1;
		str2mac(command + skip, addr);
		if (!is_valid_ether_addr(addr))
			goto out;
		netdev_info(ndev, "%s: add whitelist %pM\n", __func__, addr);
		ret = sprdwl_set_whitelist(priv, vif->ctx_id,
					   SPRDWL_SUBCMD_ADD, 1, addr);
	} else if (!strncasecmp(command, CMD_DEL_WHITELIST,
				strlen(CMD_DEL_WHITELIST))) {
		skip = strlen(CMD_DEL_WHITELIST) + 1;
		str2mac(command + skip, addr);
		if (!is_valid_ether_addr(addr))
			goto out;
		netdev_info(ndev, "%s: delete whitelist %pM\n", __func__, addr);
		ret = sprdwl_set_whitelist(priv, vif->ctx_id,
					   SPRDWL_SUBCMD_DEL, 1, addr);
	} else if (!strncasecmp(command, CMD_ENABLE_WHITELIST,
				strlen(CMD_ENABLE_WHITELIST))) {
		skip = strlen(CMD_ENABLE_WHITELIST) + 1;
		counter = command[skip];
		netdev_info(ndev, "%s: enable whitelist counter : %d\n",
			    __func__, counter);
		if (!counter) {
			ret = sprdwl_set_whitelist(priv, vif->ctx_id,
						   SPRDWL_SUBCMD_ENABLE,
						   0, NULL);
			goto out;
		}
		mac_addr = kmalloc(ETH_ALEN * counter, GFP_KERNEL);
		mac_list = mac_addr;
		if (IS_ERR(mac_addr)) {
			ret = -ENOMEM;
			goto out;
		}

		tmp = command + skip + 1;
		for (index = 0; index < counter; index++) {
			str2mac(tmp, mac_addr);
			if (!is_valid_ether_addr(mac_addr))
				goto out;
			netdev_info(ndev, "%s: enable whitelist %pM\n",
				    __func__, mac_addr);
			mac_addr += ETH_ALEN;
			tmp += 18;
		}
		ret = sprdwl_set_whitelist(priv, vif->ctx_id,
					   SPRDWL_SUBCMD_ENABLE,
					   counter, mac_list);
		kfree(mac_list);
	} else if (!strncasecmp(command, CMD_DISABLE_WHITELIST,
				strlen(CMD_DISABLE_WHITELIST))) {
		skip = strlen(CMD_DISABLE_WHITELIST) + 1;
		counter = command[skip];
		netdev_info(ndev, "%s: disable whitelist counter : %d\n",
			    __func__, counter);
		if (!counter) {
			ret = sprdwl_set_whitelist(priv, vif->ctx_id,
						   SPRDWL_SUBCMD_DISABLE,
						   0, NULL);
			goto out;
		}
		mac_addr = kmalloc(ETH_ALEN * counter, GFP_KERNEL);
		mac_list = mac_addr;
		if (IS_ERR(mac_addr)) {
			ret = -ENOMEM;
			goto out;
		}

		tmp = command + skip + 1;
		for (index = 0; index < counter; index++) {
			str2mac(tmp, mac_addr);
			if (!is_valid_ether_addr(mac_addr))
				goto out;
			netdev_info(ndev, "%s: disable whitelist %pM\n",
				    __func__, mac_addr);
			mac_addr += ETH_ALEN;
			tmp += 18;
		}
		ret = sprdwl_set_whitelist(priv, vif->ctx_id,
					   SPRDWL_SUBCMD_DISABLE,
					   counter, mac_list);
		kfree(mac_list);
	} else if (!strncasecmp(command, CMD_11V_GET_CFG,
				strlen(CMD_11V_GET_CFG))) {
		/* deflaut CP support all featrue */
		if (priv_cmd.total_len < (strlen(CMD_11V_GET_CFG) + 4)) {
			ret = -ENOMEM;
			goto out;
		}
		memset(command, 0, priv_cmd.total_len);
		if (priv->fw_std & SPRDWL_STD_11V)
			feat = priv->wnm_ft_support;

		sprintf(command, "%s %d", CMD_11V_GET_CFG, feat);
		netdev_info(ndev, "%s: get 11v feat\n", __func__);
		if (copy_to_user(priv_cmd.buf, command, priv_cmd.total_len)) {
			netdev_err(ndev, "%s: get 11v copy failed\n", __func__);
			ret = -EFAULT;
			goto out;
		}
	} else if (!strncasecmp(command, CMD_11V_SET_CFG,
				strlen(CMD_11V_SET_CFG))) {
		int skip = strlen(CMD_11V_SET_CFG) + 1;
		int cfg = command[skip];

		netdev_info(ndev, "%s: 11v cfg %d\n", __func__, cfg);
		sprdwl_set_11v_feature_support(priv, vif->ctx_id, cfg);
	} else if (!strncasecmp(command, CMD_11V_WNM_SLEEP,
				strlen(CMD_11V_WNM_SLEEP))) {
		int skip = strlen(CMD_11V_WNM_SLEEP) + 1;

		status = command[skip];
		if (status)
			interval = command[skip + 1];

		netdev_info(ndev, "%s: 11v sleep, status %d, interval %d\n",
			    __func__, status, interval);
		sprdwl_set_11v_sleep_mode(priv, vif->ctx_id, status, interval);
	} else if (!strncasecmp(command, CMD_SET_COUNTRY,
				strlen(CMD_SET_COUNTRY))) {
		skip = strlen(CMD_SET_COUNTRY) + 1;
		country = command + skip;

		if (!country || strlen(country) != SPRDWL_COUNTRY_CODE_LEN) {
			netdev_err(ndev, "%s: invalid country code\n",
				   __func__);
			ret = -EINVAL;
			goto out;
		}
		netdev_info(ndev, "%s country code:%c%c\n", __func__,
			    toupper(country[0]), toupper(country[1]));
		ret = regulatory_hint(priv->wiphy, country);
	} else if (!strncasecmp(command, CMD_SET_MAX_CLIENTS,
		   strlen(CMD_SET_MAX_CLIENTS))) {
		skip = strlen(CMD_SET_MAX_CLIENTS) + 1;
		ret = kstrtou32(command+skip, 10, &n_clients);
		if (ret < 0) {
			ret = -EINVAL;
			goto out;
		}
		ret = sprdwl_set_max_clients_allowed(priv, vif->ctx_id,
						     n_clients);
	} else {
		netdev_err(ndev, "%s command not support\n", __func__);
		ret = -ENOTSUPP;
	}
out:
	kfree(command);
	return ret;
}

static int sprdwl_set_power_save(struct net_device *ndev, struct ifreq *ifr)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_priv *priv = vif->priv;
	struct android_wifi_priv_cmd priv_cmd;
	char *command = NULL;
	int ret = 0, skip, value;

	if (!ifr->ifr_data)
		return -EINVAL;
	if (copy_from_user(&priv_cmd, ifr->ifr_data, sizeof(priv_cmd)))
		return -EFAULT;

	command = kmalloc(priv_cmd.total_len, GFP_KERNEL);
	if (!command)
		return -ENOMEM;
	if (copy_from_user(command, priv_cmd.buf, priv_cmd.total_len)) {
		ret = -EFAULT;
		goto out;
	}

	if (!strncasecmp(command, CMD_SETSUSPENDMODE,
			 strlen(CMD_SETSUSPENDMODE))) {
		skip = strlen(CMD_SETSUSPENDMODE) + 1;
		ret = kstrtoint(command + skip, 0, &value);
		if (ret)
			goto out;
		netdev_info(ndev, "%s: set suspend mode,value : %d\n",
			    __func__, value);
		ret = sprdwl_power_save(priv, vif->ctx_id,
					SPRDWL_SCREEN_ON_OFF, value);
	} else if (!strncasecmp(command, CMD_SET_FCC_CHANNEL,
				strlen(CMD_SET_FCC_CHANNEL))) {
		skip = strlen(CMD_SET_FCC_CHANNEL) + 1;
		ret = kstrtoint(command + skip, 0, &value);
		if (ret)
			goto out;
		netdev_info(ndev, "%s: set fcc channel,value : %d\n",
			    __func__, value);
		ret = sprdwl_power_save(priv, vif->ctx_id,
					SPRDWL_SET_FCC_CHANNEL, value);
	} else {
		netdev_err(ndev, "%s command not support\n", __func__);
		ret = -ENOTSUPP;
	}
out:
	kfree(command);
	return ret;
}

static int sprdwl_set_tlv(struct net_device *ndev, struct ifreq *ifr)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_priv *priv = vif->priv;
	struct android_wifi_priv_cmd priv_cmd;
	struct sprdwl_tlv_data *tlv;
	int ret;

	if (!ifr->ifr_data)
		return -EINVAL;

	if (copy_from_user(&priv_cmd, ifr->ifr_data, sizeof(priv_cmd)))
		return -EFAULT;

	if (priv_cmd.total_len < sizeof(*tlv))
		return -EINVAL;

	tlv = kmalloc(priv_cmd.total_len, GFP_KERNEL);
	if (!tlv)
		return -ENOMEM;

	if (copy_from_user(tlv, priv_cmd.buf, priv_cmd.total_len)) {
		ret = -EFAULT;
		goto out;
	}
	/*vowifi case, should send delba*/
	if (tlv->type == 1 &&
		vif->sm_state == SPRDWL_CONNECTED &&
		(is_valid_ether_addr(vif->bssid))) {
		struct sprdwl_intf *intf = NULL;
		struct sprdwl_peer_entry *peer_entry = NULL;

		peer_entry = sprdwl_find_peer_entry_using_addr(vif, vif->bssid);
		intf = (struct sprdwl_intf *)vif->priv->hw_priv;
		if (intf && peer_entry) {
			wl_info("lut:%d, vowifi_enabled, txba_map:%lu\n",
				peer_entry->lut_index,
				peer_entry->ba_tx_done_map);
			peer_entry->vowifi_enabled = 1;
			if (peer_entry->ba_tx_done_map != 0)
				sprdwl_tx_delba(intf, peer_entry, SPRDWL_AC_VO);
		}
	}

	ret = sprdwl_set_tlv_data(priv, vif->ctx_id, tlv, priv_cmd.total_len);
	if (ret)
		netdev_err(ndev, "%s set tlv(type=%#x) error\n",
			   __func__, tlv->type);
out:
	kfree(tlv);
	return ret;
}

#define SPRDWLIOCTL		(SIOCDEVPRIVATE + 1)
#define SPRDWLGETSSID		(SIOCDEVPRIVATE + 2)
#define SPRDWLSETFCC		(SIOCDEVPRIVATE + 3)
#define SPRDWLSETSUSPEND	(SIOCDEVPRIVATE + 4)
#define SPRDWLSETCOUNTRY	(SIOCDEVPRIVATE + 5)
#define SPRDWLSETTLV		(SIOCDEVPRIVATE + 7)

static int sprdwl_ioctl(struct net_device *ndev, struct ifreq *req, int cmd)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct iwreq *wrq = (struct iwreq *)req;

	switch (cmd) {
	case SPRDWLIOCTL:
	case SPRDWLSETCOUNTRY:
		return sprdwl_priv_cmd(ndev, req);
	case SPRDWLGETSSID:
		if (vif->ssid_len > 0) {
			if (copy_to_user(wrq->u.essid.pointer, vif->ssid,
					 vif->ssid_len))
				return -EFAULT;
			wrq->u.essid.length = vif->ssid_len;
		} else {
			netdev_err(ndev, "SSID len is zero\n");
			return -EFAULT;
		}
		break;
	case SPRDWLSETFCC:
	case SPRDWLSETSUSPEND:
		return sprdwl_set_power_save(ndev, req);
	case SPRDWLSETTLV:
		return sprdwl_set_tlv(ndev, req);
	default:
		netdev_err(ndev, "Unsupported IOCTL %d\n", cmd);
		return -ENOTSUPP;
	}

	return 0;
}

static bool mc_address_changed(struct net_device *ndev)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct netdev_hw_addr *ha;
	u8 mc_count, index;
	u8 *mac_addr;
	bool found;

	mc_count = netdev_mc_count(ndev);

	if (mc_count != vif->mc_filter->mac_num)
		return true;

	mac_addr = vif->mc_filter->mac_addr;
	netdev_for_each_mc_addr(ha, ndev) {
		found = false;
		for (index = 0; index < vif->mc_filter->mac_num; index++) {
			if (!memcmp(ha->addr, mac_addr, ETH_ALEN)) {
				found = true;
				break;
			}
			mac_addr += ETH_ALEN;
		}

		if (!found)
			return true;
	}
	return false;
}

#define SPRDWL_RX_MODE_MULTICAST	1
static void sprdwl_set_multicast(struct net_device *ndev)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_priv *priv = vif->priv;
	struct sprdwl_work *work;
	struct netdev_hw_addr *ha;
	u8 mc_count;
	u8 *mac_addr;

	mc_count = netdev_mc_count(ndev);
	netdev_info(ndev, "%s multicast address num: %d\n", __func__, mc_count);
	if (mc_count > priv->max_mc_mac_addrs)
		return;

	vif->mc_filter->mc_change = false;
	if ((ndev->flags & IFF_MULTICAST) && (mc_address_changed(ndev))) {
		mac_addr = vif->mc_filter->mac_addr;
		netdev_for_each_mc_addr(ha, ndev) {
			netdev_info(ndev, "%s set mac: %pM\n", __func__,
				    ha->addr);
			if ((ha->addr[0] != 0x33 || ha->addr[1] != 0x33) &&
			    (ha->addr[0] != 0x01 || ha->addr[1] != 0x00 ||
			     ha->addr[2] != 0x5e || ha->addr[3] > 0x7f)) {
				netdev_info(ndev, "%s invalid addr\n",
					    __func__);
				return;
			}
			ether_addr_copy(mac_addr, ha->addr);
			mac_addr += ETH_ALEN;
		}
		vif->mc_filter->mac_num = mc_count;
		vif->mc_filter->mc_change = true;
	} else if (!(ndev->flags & IFF_MULTICAST) && vif->mc_filter->mac_num) {
		vif->mc_filter->mac_num = 0;
		vif->mc_filter->mc_change = true;
	}

	work = sprdwl_alloc_work(0);
	if (!work) {
		netdev_err(ndev, "%s out of memory\n", __func__);
		return;
	}
	work->vif = vif;
	work->id = SPRDWL_WORK_MC_FILTER;
	vif->mc_filter->subtype = SPRDWL_RX_MODE_MULTICAST;
	sprdwl_queue_work(vif->priv, work);
}

static struct net_device_ops sprdwl_netdev_ops = {
	.ndo_init = sprdwl_init,
	.ndo_uninit = sprdwl_uninit,
	.ndo_open = sprdwl_open,
	.ndo_stop = sprdwl_close,
	.ndo_start_xmit = sprdwl_start_xmit,
	.ndo_get_stats = sprdwl_get_stats,
	.ndo_tx_timeout = sprdwl_tx_timeout,
	.ndo_do_ioctl = sprdwl_ioctl
};

static int sprdwl_inetaddr_event(struct notifier_block *this,
				 unsigned long event, void *ptr)
{
	struct net_device *ndev;
	struct sprdwl_vif *vif;
	struct sprdwl_peer_entry *entry;
	struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;

	if (!ifa || !(ifa->ifa_dev->dev))
		return NOTIFY_DONE;
	if (ifa->ifa_dev->dev->netdev_ops != &sprdwl_netdev_ops)
		return NOTIFY_DONE;

	ndev = ifa->ifa_dev->dev;
	vif = netdev_priv(ndev);

	switch (vif->wdev.iftype) {
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_P2P_CLIENT:
		if (event == NETDEV_UP) {
			entry = sprdwl_find_peer_entry_using_addr(vif,
								  vif->bssid);
			if (entry != NULL) {
				if (entry->ctx_id == vif->ctx_id)
					entry->ip_acquired = 1;
				else
					wl_err("ctx_id(%d) mismatch\n",
					       entry->ctx_id);
			} else {
			    wl_err("failed to find entry\n");
			}

			sprdwl_notify_ip(vif->priv, vif->ctx_id, SPRDWL_IPV4,
					 (u8 *)&ifa->ifa_address);
		}

		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block sprdwl_inetaddr_cb = {
	.notifier_call = sprdwl_inetaddr_event
};

static int sprdwl_inetaddr6_event(struct notifier_block *this,
				  unsigned long event, void *ptr)
{
	struct net_device *ndev;
	struct sprdwl_vif *vif;
	struct inet6_ifaddr *inet6_ifa = (struct inet6_ifaddr *)ptr;
	struct sprdwl_work *work;
	u8 *ipv6_addr;

	if (!inet6_ifa || !(inet6_ifa->idev->dev))
		return NOTIFY_DONE;

	if (inet6_ifa->idev->dev->netdev_ops != &sprdwl_netdev_ops)
		return NOTIFY_DONE;

	ndev = inet6_ifa->idev->dev;
	vif = netdev_priv(ndev);

	switch (vif->wdev.iftype) {
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_P2P_CLIENT:
		if (event == NETDEV_UP) {
			work = sprdwl_alloc_work(SPRDWL_IPV6_ADDR_LEN);
			if (!work) {
				netdev_err(ndev, "%s out of memory\n",
					   __func__);
				return NOTIFY_DONE;
			}
			work->vif = vif;
			work->id = SPRDWL_WORK_NOTIFY_IP;
			ipv6_addr = (u8 *)work->data;
			memcpy(ipv6_addr, (u8 *)&inet6_ifa->addr,
			       SPRDWL_IPV6_ADDR_LEN);
			sprdwl_queue_work(vif->priv, work);
		}
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block sprdwl_inet6addr_cb = {
	.notifier_call = sprdwl_inetaddr6_event
};

#define WIFI_MAC_ADDR_PATH "/data/misc/wifi/wifimac.txt"
#define WIFI_MAC_ADDR_TEMP "/data/misc/wifi/wifimac_temp.txt"

static int sprdwl_get_mac_from_file(struct sprdwl_vif *vif, u8 *addr)
{
	struct file *fp = 0;
	u8 buf[64] = { 0 };
	mm_segment_t fs;
	loff_t *pos;

	fp = filp_open(WIFI_MAC_ADDR_PATH, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		wl_err("WIFI MAC can't be found wifimac.txt!\n");
		fp = filp_open(WIFI_MAC_ADDR_TEMP, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			wl_err("WIFI MAC can't found in temp file!\n");
			return -ENOENT;
		}
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = &fp->f_pos;
	vfs_read(fp, buf, sizeof(buf), pos);

	filp_close(fp, NULL);
	set_fs(fs);

	str2mac(buf, addr);
	if (!is_valid_ether_addr(addr)) {
		netdev_err(vif->ndev, "%s invalid MAC address (%pM)\n",
				 __func__, addr);
		return -EINVAL;
	}
	if (is_local_ether_addr(addr)) {
		netdev_warn(vif->ndev, "%s Warning: Assigning a locally valid "
				 "MAC address (%pM) to a device\n",
				 __func__, addr);
		netdev_warn(vif->ndev, "%s You should not set the 2nd rightmost "
				"bit in the first byte of the MAC\n", __func__);
		vif->local_mac_flag = 1;
	} else
		vif->local_mac_flag = 0;

	return 0;
}

static int write_mac_addr(u8 *addr)
{
	struct file *fp = 0;
	mm_segment_t old_fs;
	char buf[18];
	loff_t pos = 0;
	/*open file*/
	fp = filp_open(WIFI_MAC_ADDR_TEMP, O_CREAT|O_RDWR, 777);
	if (IS_ERR(fp)) {
		 wl_err("can't create WIFI MAC file!\n");
		 return -ENOENT;
	 }
	 /*format MAC address*/
	 sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1],
		     addr[2], addr[3], addr[4], addr[5]);
	 /*save old fs: should be USER_DS*/
	 old_fs = get_fs();
	 /*change it to KERNEL_DS*/
	 set_fs(KERNEL_DS);
	 /*write file*/
	 vfs_write(fp, buf, sizeof(buf), &pos);
	 /*close file*/
	 filp_close(fp, NULL);
	 /*restore to old fs*/
	 set_fs(old_fs);

	 return 0;
}

static void sprdwl_set_mac_addr(struct sprdwl_vif *vif, u8 *pending_addr,
				u8 *addr)
{
	enum nl80211_iftype type = vif->wdev.iftype;
	struct sprdwl_priv *priv = vif->priv;

	if (!addr) {
		return;
	} else if (pending_addr && is_valid_ether_addr(pending_addr)) {
		ether_addr_copy(addr, pending_addr);
	} else if (is_valid_ether_addr(priv->default_mac)) {
		ether_addr_copy(addr, priv->default_mac);
	} else if (sprdwl_get_mac_from_file(vif, addr)) {
		random_ether_addr(addr);
		netdev_warn(vif->ndev, "%s Warning: use random MAC address\n",
				  __func__);
		/* initialize MAC addr with specific OUI */
		addr[0] = 0x40;
		addr[1] = 0x45;
		addr[2] = 0xda;
		/*write random mac to WIFI FILE*/
		write_mac_addr(addr);
	}

	switch (type) {
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_AP:
		ether_addr_copy(priv->default_mac, addr);
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_P2P_GO:
		addr[4] ^= 0x80;
	case NL80211_IFTYPE_P2P_DEVICE:
		addr[0] ^= 0x02;
		break;
	default:
		break;
	}
}
void init_scan_list(struct sprdwl_vif *vif)
{
	if (!list_empty(&vif->scan_head_ptr)) {
		/*clean scan list if not empty first*/
		clean_scan_list(vif);
	}
	INIT_LIST_HEAD(&vif->scan_head_ptr);
}

void clean_scan_list(struct sprdwl_vif *vif)
{
	struct scan_result *node, *pos;
	int count = 0;

	list_for_each_entry_safe(node, pos, &vif->scan_head_ptr, list) {
		list_del(&node->list);
		kfree(node);
		count++;
	}
	pr_err("delete scan node num:%d\n", count);
}

#ifdef ACS_SUPPORT
void clean_survey_info_list(struct sprdwl_vif *vif)
{
	struct sprdwl_bssid *bssid = NULL, *pos_bssid = NULL;
	struct sprdwl_survey_info *info = NULL, *pos_info = NULL;

	list_for_each_entry_safe(info, pos_info,
				 &vif->survey_info_list, survey_list) {
		list_del(&info->survey_list);

		if (!list_empty(&info->bssid_list)) {
			list_for_each_entry_safe(bssid, pos_bssid,
						 &info->bssid_list, list) {
				list_del(&bssid->list);
				kfree(bssid);
				bssid = NULL;
			}
		}

		kfree(info);
		info = NULL;
	}
}

static unsigned short cal_total_beacon(struct sprdwl_vif *vif,
				       struct sprdwl_survey_info *info)
{
	unsigned short total_beacon = 0;
	short pos_chan, chan;

	total_beacon += info->beacon_num;
	chan = (short)info->chan;

	if (chan > 0 && chan < 15) {
		/* Calculate overlapping channels */
		list_for_each_entry(info, &vif->survey_info_list, survey_list) {
			pos_chan = (short)info->chan;
			if (pos_chan > (chan - 4) && pos_chan < (chan + 4) &&
			    pos_chan != chan) {
				total_beacon += info->beacon_num;
			}
		}
	}

	netdev_info(vif->ndev, "survey chan: %d, total beacon: %d!\n",
		    chan, total_beacon);
	return total_beacon;
}

/* Transfer beacons to survey info */
void transfer_survey_info(struct sprdwl_vif *vif)
{
	struct ieee80211_channel *channel = NULL;
	struct wiphy *wiphy = vif->wdev.wiphy;
	struct sprdwl_survey_info *info = NULL;
	unsigned int freq;
	unsigned short total_beacon = 0;

	list_for_each_entry(info, &vif->survey_info_list, survey_list) {
		freq = ieee80211_channel_to_frequency(info->chan,
				info->chan <= CH_MAX_2G_CHANNEL ?
				IEEE80211_BAND_2GHZ : IEEE80211_BAND_5GHZ);
		channel = ieee80211_get_channel(wiphy, freq);
		if (channel) {
			total_beacon = cal_total_beacon(vif, info);
			info->cca_busy_time =
				(total_beacon < 20) ? total_beacon : 20;
			info->noise =
				-95 + ((total_beacon < 30) ? total_beacon : 30);
			info->channel = channel;
		}

		freq = 0;
		channel = NULL;
		total_beacon = 0;
	}
}

static bool find_bssid(struct sprdwl_survey_info *info, unsigned char *nbssid)
{
	struct sprdwl_bssid *bssid = NULL;
	int ret = false;

	if (!list_empty(&info->bssid_list)) {
		list_for_each_entry(bssid, &info->bssid_list, list) {
			if (!memcmp(bssid->bssid, nbssid, 6)) {
				ret = true;
				break;
			}
		}
	}

	return ret;
}

static struct sprdwl_survey_info *find_survey_info(struct sprdwl_vif *vif,
						   unsigned short chan)
{
	struct sprdwl_survey_info *info = NULL, *result = NULL;

	if (!list_empty(&vif->survey_info_list)) {
		list_for_each_entry(info, &vif->survey_info_list, survey_list) {
			if (chan == info->chan) {
				result = info;
				break;
			}
		}
	}

	return result;
}

void acs_scan_result(struct sprdwl_vif *vif, u16 chan,
		     struct ieee80211_mgmt *mgmt)
{
	struct sprdwl_survey_info *info = NULL;
	struct sprdwl_bssid *bssid = NULL;

	info = find_survey_info(vif, chan);
	if (info) {
		if (!find_bssid(info, mgmt->bssid)) {
			bssid = kmalloc(sizeof(*bssid), GFP_KERNEL);
			if (bssid) {
				ether_addr_copy(bssid->bssid, mgmt->bssid);
				list_add_tail(&bssid->list, &info->bssid_list);
				info->beacon_num++;
			} else {
				netdev_err(vif->ndev, "%s no memory for bssid!\n",
					   __func__);
			}
		}
	}
}
#endif /* ACS_SUPPORT */

static void sprdwl_init_vif(struct sprdwl_priv *priv, struct sprdwl_vif *vif,
			    const char *name)
{
	WARN_ON(strlen(name) >= sizeof(vif->name));

	strcpy(vif->name, name);
	vif->priv = priv;
	vif->sm_state = SPRDWL_DISCONNECTED;
#ifdef ACS_SUPPORT
	/* Init ACS */
	INIT_LIST_HEAD(&vif->survey_info_list);
#endif
	INIT_LIST_HEAD(&vif->scan_head_ptr);
}

static void sprdwl_deinit_vif(struct sprdwl_vif *vif)
{
	sprdwl_scan_done(vif, true);
	sprdwl_sched_scan_done(vif, true);
	/* We have to clear all the work which
	 * is belong to the vif we are going to remove.
	 */
	sprdwl_cancle_work(vif->priv, vif);

	if (vif->ref > 0) {
		int cnt = 0;
		unsigned long timeout = jiffies + msecs_to_jiffies(1000);

		do {
			usleep_range(2000, 2500);
			cnt++;
			if (time_after(jiffies, timeout)) {
				netdev_err(vif->ndev, "%s timeout cnt %d\n",
					   __func__, cnt);
				break;
			}
		} while (vif->ref > 0);
		netdev_dbg(vif->ndev, "cnt %d\n", cnt);
	}
}

static struct sprdwl_vif *sprdwl_register_wdev(struct sprdwl_priv *priv,
					       const char *name,
					       enum nl80211_iftype type,
					       u8 *addr)
{
	struct sprdwl_vif *vif;
	struct wireless_dev *wdev;

	vif = kzalloc(sizeof(*vif), GFP_KERNEL);
	if (!vif)
		return ERR_PTR(-ENOMEM);

	/* initialize vif stuff */
	sprdwl_init_vif(priv, vif, name);

	/* initialize wdev stuff */
	wdev = &vif->wdev;
	wdev->wiphy = priv->wiphy;
	wdev->iftype = type;

	sprdwl_set_mac_addr(vif, addr, wdev->address);
	wl_info("iface '%s'(%pM) type %d added\n", name, wdev->address, type);

	return vif;
}

static void sprdwl_unregister_wdev(struct sprdwl_vif *vif)
{
	wl_info("iface '%s' deleted\n", vif->name);

	cfg80211_unregister_wdev(&vif->wdev);
	sprdwl_deinit_vif(vif);
	kfree(vif);
}

static struct sprdwl_vif *sprdwl_register_netdev(struct sprdwl_priv *priv,
						 const char *name,
						 enum nl80211_iftype type,
						 u8 *addr)
{
	struct net_device *ndev;
	struct wireless_dev *wdev;
	struct sprdwl_vif *vif;
	int ret;

	ndev = alloc_netdev(sizeof(*vif), name, NET_NAME_UNKNOWN, ether_setup);
	if (!ndev) {
		wl_err("%s failed to alloc net_device!\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	/* initialize vif stuff */
	vif = netdev_priv(ndev);
	vif->ndev = ndev;
	sprdwl_init_vif(priv, vif, name);

	/* initialize wdev stuff */
	wdev = &vif->wdev;
	wdev->netdev = ndev;
	wdev->wiphy = priv->wiphy;
	wdev->iftype = type;

	/* initialize ndev stuff */
	ndev->ieee80211_ptr = wdev;
	if (priv->fw_capa & SPRDWL_CAPA_MC_FILTER) {
		wl_info("\tMulticast Filter supported\n");
		vif->mc_filter =
		    kzalloc(sizeof(struct sprdwl_mc_filter) +
			    priv->max_mc_mac_addrs * ETH_ALEN, GFP_KERNEL);
		if (!vif->mc_filter) {
			ret = -ENOMEM;
			goto err;
		}

		sprdwl_netdev_ops.ndo_set_rx_mode = sprdwl_set_multicast;
	}
	ndev->netdev_ops = &sprdwl_netdev_ops;
	ndev->destructor = free_netdev;
	ndev->needed_headroom = priv->skb_head_len;
	ndev->watchdog_timeo = 2 * HZ;
	ndev->features |= NETIF_F_ALL_CSUM;
#ifdef SC2355_RX_NAPI
	ndev->features |= NETIF_F_GRO;
#endif
	SET_NETDEV_DEV(ndev, wiphy_dev(priv->wiphy));

	sprdwl_set_mac_addr(vif, addr, ndev->dev_addr);

	/* register new Ethernet interface */
	ret = register_netdevice(ndev);
	if (ret) {
		netdev_err(ndev, "failed to regitster netdev(%d)!\n", ret);
		goto err;
	}

	wl_info("iface '%s'(%pM) type %d added\n",
		ndev->name, ndev->dev_addr, type);
	return vif;
err:
	sprdwl_deinit_vif(vif);
	free_netdev(ndev);
	return ERR_PTR(ret);
}

static void sprdwl_unregister_netdev(struct sprdwl_vif *vif)
{
	wl_info("iface '%s' deleted\n", vif->ndev->name);

	if (vif->priv->fw_capa & SPRDWL_CAPA_MC_FILTER)
		kfree(vif->mc_filter);
	sprdwl_deinit_vif(vif);
	unregister_netdevice(vif->ndev);
}

struct wireless_dev *sprdwl_add_iface(struct sprdwl_priv *priv,
				      const char *name,
				      enum nl80211_iftype type, u8 *addr)
{
	struct sprdwl_vif *vif;

	if (type == NL80211_IFTYPE_P2P_DEVICE)
		vif = sprdwl_register_wdev(priv, name, type, addr);
	else
		vif = sprdwl_register_netdev(priv, name, type, addr);

	if (IS_ERR(vif)) {
		wl_err("failed to add iface '%s'\n", name);
		return (void *)vif;
	}
#ifdef DFS_MASTER
	sprdwl_init_dfs_master(vif);
#endif

	spin_lock_bh(&priv->list_lock);
	list_add_tail(&vif->vif_node, &priv->vif_list);
	spin_unlock_bh(&priv->list_lock);

	return &vif->wdev;
}

int sprdwl_del_iface(struct sprdwl_priv *priv, struct sprdwl_vif *vif)
{
#ifdef DFS_MASTER
	sprdwl_deinit_dfs_master(vif);
#endif
	if (!vif->ndev)
		sprdwl_unregister_wdev(vif);
	else
		sprdwl_unregister_netdev(vif);

	return 0;
}

static void sprdwl_del_all_ifaces(struct sprdwl_priv *priv)
{
	struct sprdwl_vif *vif;

next_intf:
	spin_lock_bh(&priv->list_lock);
	list_for_each_entry_reverse(vif, &priv->vif_list, vif_node) {
		list_del(&vif->vif_node);
		spin_unlock_bh(&priv->list_lock);
		rtnl_lock();
		sprdwl_del_iface(priv, vif);
		rtnl_unlock();
		goto next_intf;
	}

	spin_unlock_bh(&priv->list_lock);

}

static void sprdwl_init_debugfs(struct sprdwl_priv *priv)
{
	if (!priv->wiphy->debugfsdir)
		return;
	priv->debugfs = debugfs_create_dir("sprdwl_wifi",
					   priv->wiphy->debugfsdir);
	if (IS_ERR_OR_NULL(priv->debugfs))
		return;
	sprdwl_intf_debugfs(priv, priv->debugfs);
}

int sprdwl_core_init(struct device *dev, struct sprdwl_priv *priv)
{
	struct wiphy *wiphy = priv->wiphy;
	struct wireless_dev *wdev;
	int ret;
	ret = sprdwl_sync_version(priv);
	if (ret) {
		wl_err("SYNC CMD ERROR!!\n");
		goto out;
	}
	sprdwl_download_ini(priv);
	sprdwl_tcp_ack_init(priv);
	sprdwl_get_fw_info(priv);
#ifdef RTT_SUPPORT
	sprdwl_ftm_init(priv);
#endif /* RTT_SUPPORT */
	sprdwl_setup_wiphy(wiphy, priv);
	sprdwl_vendor_init(wiphy);
	set_wiphy_dev(wiphy, dev);
	ret = wiphy_register(wiphy);
	if (ret) {
		wl_err("failed to regitster wiphy(%d)!\n", ret);
		goto out;
	}
	sprdwl_init_debugfs(priv);

	rtnl_lock();
	wdev = sprdwl_add_iface(priv, "wlan%d", NL80211_IFTYPE_STATION, NULL);
	rtnl_unlock();
	if (IS_ERR(wdev)) {
		wiphy_unregister(wiphy);
		ret = -ENXIO;
		goto out;
	}

#ifdef SC2355_RX_NAPI
	sprdwl_rx_napi_init(wdev->netdev,
			    ((struct sprdwl_intf *)priv->hw_priv));
#endif

#if defined(SC2355_FTR)
	qos_enable(1);
#endif
	sprdwl_init_npi();
	ret = register_inetaddr_notifier(&sprdwl_inetaddr_cb);
	if (ret)
		wl_err("%s failed to register inetaddr notifier(%d)!\n",
		       __func__, ret);
	if (priv->fw_capa & SPRDWL_CAPA_NS_OFFLOAD) {
		wl_info("\tIPV6 NS Offload supported\n");
		ret = register_inet6addr_notifier(&sprdwl_inet6addr_cb);
		if (ret)
			wl_err("%s failed to register inet6addr notifier(%d)!\n",
			       __func__, ret);
	}

	trace_info_init();

out:
	return ret;
}

int sprdwl_core_deinit(struct sprdwl_priv *priv)
{
	unregister_inetaddr_notifier(&sprdwl_inetaddr_cb);
	if (priv->fw_capa & SPRDWL_CAPA_NS_OFFLOAD)
		unregister_inet6addr_notifier(&sprdwl_inet6addr_cb);
	sprdwl_deinit_npi();
#if defined(SC2355_FTR)
	qos_enable(0);
#endif
	sprdwl_del_all_ifaces(priv);
	sprdwl_vendor_deinit(priv->wiphy);
	wiphy_unregister(priv->wiphy);
	sprdwl_cmd_wake_upall();
	sprdwl_tcp_ack_deinit(priv);
#ifdef RTT_SUPPORT
	sprdwl_ftm_deinit(priv);
#endif /* RTT_SUPPORT */
	trace_info_deinit();

	return 0;
}

unsigned int wfa_cap;
module_param(wfa_cap, uint, 0);
MODULE_PARM_DESC(wfa_cap, "set capability for WFA test");

unsigned int tcp_ack_drop_cnt = SPRDWL_TCP_ACK_DROP_CNT;
/* Maybe you need S_IRUGO | S_IWUSR for debug */
module_param(tcp_ack_drop_cnt, uint, 0);
MODULE_PARM_DESC(tcp_ack_drop_cnt, "valid values: [1, 13]");

#ifdef TCP_ACK_DROP_SUPPORT
unsigned int tcp_ack_drop_enable = 1;
module_param(tcp_ack_drop_enable, uint, 0);
MODULE_PARM_DESC(tcp_ack_drop_enable, "valid values: [0, 1]");
#else
const unsigned int tcp_ack_drop_enable;
#endif

