/** Filename: tfasoftdsp_tfafieldnames.h
 *
 */

#ifndef _TFASOFTDSP_TFAFIELDNAMES_H
#define _TFASOFTDSP_TFAFIELDNAMES_H

//#define I2CVERSION    9.1

//typedef enum nxpTfasoftdspBfEnumList {
//
//} nxpTfasoftdspBfEnumList_t;
#define TFASOFTDSP_NAMETABLE static tfaBfName_t TfasoftdspDatasheetNames[]= {\
		{0x000f, "AUDFS"}, \
		{0x010f, "BATS"}, \
		{0x030f, "REV"}, \
		{0x040f, "BSSS"}, \
		{0x050f, "DCVO"}, \
		{0x060f, "BSSBY"}, \
		{0x070f, "MTPOTC"}, \
		{0x080f, "R25CL"}, \
		{0x090f, "R25CR"}, \
		{0x0a0f, "MTPEX"}, \
		{0x100f, "CFSML"}, \
		{0x110f, "CFSMR"}, \
		{0x130f, "RST"}, \
		{0x150f, "VOL"}, \
		{0x170f, "SBSL"}, \
		{0x190f, "DCDIS"}, \
		{0x1a0f, "DCA"}, \
		{0x1b0f, "SWS"}, \
		{0x1c0f, "MANMUTE"}, \
		{0x260f, "MTPRDLSB"}, \
		{0x270f, "MTPRDMSB"}, \
		{0x280f, "BSSR"}, \
		{0x290f, "TEMPS"}, \
		{0x2c0f, "BSST"}, \
		{0x2d0f, "BSSRL"}, \
		{0x2e0f, "BSSRR"}, \
		{0x2f0f, "BSSHY"}, \
		{0x300f, "NPA"}, /* new_params_available */\
   { 0xffff,"Unknown bitfield enum" }   /* not found */\
};

#define TFASOFTDSP_BITNAMETABLE static tfaBfName_t TfasoftdspBitNames[]= {\
		{0x000f, "audio_fs"}, \
		{0x010f, "bat_adc"}, \
		{0x020f, "bat_prot_config_all"}, \
		{0x030f, "device_rev"}, \
		{0x040f, "batsense_steepness"}, \
		{0x050f, "boost_volt"}, \
		{0x060f, "bypass_clipper"}, \
		{0x070f, "calibration_onetime"}, \
		{0x080f, "calibr_R25C_L"}, \
		{0x090f, "calibr_R25C_R"}, \
		{0x0a0f, "calibr_ron_done"}, \
		{0x0b0f, "cf_cold_started"}, \
		{0x0c0f, "cf_enbl_amplifier"}, \
		{0x0d0f, "cf_flag_cf_speakererror"}, \
		{0x0e0f, "cf_flag_cf_speakererror_left"}, \
		{0x0f0f, "cf_flag_cf_speakererror_right"}, \
		{0x100f, "cf_mute_left"}, \
		{0x110f, "cf_mute_right"}, \
		{0x120f, "cf_reset_min_vbat"}, \
		{0x130f, "cf_rst_dsp"}, \
		{0x140f, "cf_sel_vbat"}, \
		{0x150f, "cf_volume"}, \
		{0x160f, "cf_volume_sec"}, \
		{0x170f, "coolflux_configured"}, \
		{0x180f, "ctrl_supplysense"}, \
		{0x190f, "dcdcoff_mode"}, \
		{0x1a0f, "enbl_boost"}, \
		{0x1b0f, "flag_engage"}, \
		{0x1c0f, "flag_man_start_mute_audio"}, \
		{0x1d0f, "auto_copy_mtp_to_iic"}, \
		{0x1e0f, "man_copy_iic_to_mtp"}, \
		{0x1f0f, "man_copy_mtp_to_iic"}, \
		{0x200f, "mtp_man_address_in"}, \
		{0x210f, "mtp_man_address_out"}, \
		{0x220f, "mtp_man_data_in"}, \
		{0x230f, "mtp_man_data_in_lsb"}, \
		{0x240f, "mtp_man_data_in_msb"}, \
		{0x250f, "mtp_man_data_out"}, \
		{0x260f, "mtp_man_data_out_lsb"}, \
		{0x270f, "mtp_man_data_out_msb"}, \
		{0x280f, "sel_vbat"}, \
		{0x290f, "temp_adc"}, \
		{0x2a0f, "test_abistfft_enbl"}, \
		{0x2b0f, "type_bits_fw"}, \
		{0x2c0f, "vbat_prot_thlevel"}, \
		{0x2d0f, "vbat_prot_max_reduct"}, \
		{0x2e0f, "vbat_prot_release_time"}, \
		{0x2f0f, "vbat_prot_hysterese"}, \
		{0x300f, "new_params_available"}, \
   { 0xffff,"Unknown bitfield enum" }    /* not found */ \
};

//enum tfasoftdsp_irq {
//	tfasoftdsp_irq_all = -1 /* all irqs */};
//
//#define TFASOFTDSP_IRQ_NAMETABLE static tfaIrqName_t TfasoftdspIrqNames[]= {};

#endif /* _TFASOFTDSP_TFAFIELDNAMES_H */
