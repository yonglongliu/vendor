# !/bin/sh
insmod /lib/modules/sprdwl_ng.ko
sleep 2
iwnpi wlan0 start
sleep 2
#iwnpi wlan0 stop
iwnpi wlan0 set_mac 11:22:33:44:55:66
sleep 2
iwnpi wlan0 get_mac
sleep 2
iwnpi wlan0 set_macfilter 4
sleep 2
iwnpi wlan0 get_macfilter
sleep 2
iwnpi wlan0 set_channel 3 3
sleep 2
iwnpi wlan0 get_channel
sleep 2
iwnpi wlan0 get_rssi
sleep 2
iwnpi wlan0 set_tx_mode 1
sleep 2
iwnpi wlan0 get_tx_mode
sleep 2
iwnpi wlan0 set_rate 1
sleep 2
iwnpi wlan0 get_rate
sleep 2
sleep 2
#iwnpi wlan0 set_band <NUM>
#iwnpi wlan0 get_band
iwnpi wlan0 set_cbw 1
sleep 2
iwnpi wlan0 get_cbw
sleep 2
iwnpi wlan0 set_pkt_len 1000
sleep 2
iwnpi wlan0 get_pkt_len
sleep 2
iwnpi wlan0 set_preamble 1
sleep 2
iwnpi wlan0 set_gi 1
sleep 2
iwnpi wlan0 get_gi
sleep 2
iwnpi wlan0 set_payload 1000
sleep 2
iwnpi wlan0 get_payload
sleep 2
iwnpi wlan0 set_tx_power 100
sleep 2
iwnpi wlan0 get_tx_power
sleep 2
iwnpi wlan0 set_tx_count 1000
sleep 2
iwnpi wlan0 get_rx_ok
sleep 2
iwnpi wlan0 tx_start
sleep 2
iwnpi wlan0 tx_stop
sleep 2
iwnpi wlan0 rx_start
sleep 2
iwnpi wlan0 rx_stop
sleep 2
#iwnpi wlan0 get_reg <mac/phy0/phy1/rf> <addr_offset> [count]
sleep 2
#iwnpi wlan0 set_reg <mac/phy0/phy1/rf> <addr_offset> <value>
sleep 2
iwnpi wlan0 sin_wave
sleep 2
iwnpi wlan0 lna_on
sleep 2
iwnpi wlan0 lna_off
sleep 2
iwnpi wlan0 lna_status
sleep 2
iwnpi wlan0 set_wlan_cap 9
sleep 2
iwnpi wlan0 get_wlan_cap
sleep 2
iwnpi wlan0 conn_status #get current connected ap's info
sleep 2
iwnpi wlan0 get_reconnect #get reconnect times
sleep 2
iwnpi wlan0 mcs #get mcs index in use.
sleep 2
iwnpi wlan0 set_ar 1 1 1
sleep 2
iwnpi wlan0 get_ar
sleep 2
iwnpi wlan0 set_ar_pktcnt 1000 #set auto rate pktcnt.
sleep 2
iwnpi wlan0 get_ar_pktcnt
sleep 2
iwnpi wlan0 set_ar_retcnt 1000
sleep 2
iwnpi wlan0 get_ar_retcnt
sleep 2
#iwnpi wlan0 roam <channel> <mac_addr> <ssid>
sleep 2
#iwnpi wlan0 set_wmm [be|bk|vi|vo] {cwmin} {cwmax} {aifs} {txop}
sleep 2
iwnpi wlan0 set_eng_mode 1 1
sleep 2
iwnpi wlan0 get_sup_ch
sleep 2
iwnpi wlan0 get_pa_info
sleep 2
iwnpi wlan0 get_tx_status
sleep 2
iwnpi wlan0 get_rx_status
sleep 2
iwnpi wlan0 get_lut_status
sleep 2
iwnpi wlan0 open_pkt_log
sleep 2
iwnpi wlan0 close_pkt_log
sleep 2
iwnpi wlan0 set_chain 2
sleep 2
iwnpi wlan0 get_chain
sleep 2
iwnpi wlan0 set_sbw 1
sleep 2
iwnpi wlan0 get_sbw
sleep 2
iwnpi wlan0 set_ampdu_en
sleep 2
iwnpi wlan0 get_ampdu_en
sleep 2
iwnpi wlan0 set_ampdu_cnt
sleep 2
iwnpi wlan0 get_ampdu_cnt
sleep 2
iwnpi wlan0 set_stbc
sleep 2
iwnpi wlan0 get_stbc
sleep 2
iwnpi wlan0 set_fec 1
sleep 2
iwnpi wlan0 get_fec
sleep 2
iwnpi wlan0 get_rx_snr
sleep 2
iwnpi wlan0 set_force_txgain
sleep 2
iwnpi wlan0 set_force_rxgain
sleep 2
iwnpi wlan0 set_bfee_en
sleep 2
iwnpi wlan0 set_dpd_en
sleep 2
iwnpi wlan0 set_agc_log_en
sleep 2
iwnpi wlan0 set_efuse
sleep 2
iwnpi wlan0 get_efuse
sleep 2
iwnpi wlan0 set_txs_cal_res
sleep 2
iwnpi wlan0 get_txs_temp
sleep 2
iwnpi wlan0 set_cbank_reg
sleep 2
iwnpi wlan0 set_11n_green
sleep 2
iwnpi wlan0 get_11n_green
sleep 2
iwnpi wlan0 atenna_coup
sleep 2
iwnpi wlan0 set_tx_rf_gain
sleep 2
iwnpi wlan0 set_tx_pa_bias
sleep 2
iwnpi wlan0 set_tx_dvga_gain
sleep 2
iwnpi wlan0 set_rx_lna_gain
sleep 2
iwnpi wlan0 set_rx_vga_gain
sleep 2
iwnpi wlan0 set_mac_bssid
sleep 2
iwnpi wlan0 get_mac_bssid
sleep 2
iwnpi wlan0 force_tx_rate
sleep 2
iwnpi wlan0 force_tx_power
sleep 2
iwnpi wlan0 en_fw_loop
sleep 2
iwnpi wlan0 set_prot_mode 1 2
sleep 2
iwnpi wlan0 get_prot_mode  1
sleep 2
iwnpi wlan0 set_threshold 1 1 1
sleep 2
iwnpi wlan0 set_ar_dbg
sleep 2
