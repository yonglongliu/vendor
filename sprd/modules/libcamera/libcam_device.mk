TUNING_BIN := s5k3l2xx_mipi_raw_3a.bin
TUNING_BIN += s5k3l2xx_mipi_raw_shading.bin
TUNING_BIN += s5k3l2xx_mipi_raw_caf.bin
TUNING_BIN += s5k5e3yx_mipi_raw_3a.bin
TUNING_BIN += s5k5e3yx_mipi_raw_shading.bin
TUNING_BIN += s5k5e3yx_mipi_raw_caf.bin
TUNING_BIN += imx132_mipi_raw_3a.bin
TUNING_BIN += imx132_mipi_raw_shading.bin
TUNING_BIN += imx132_mipi_raw_caf.bin
TUNING_BIN += s5k3p3sm_mipi_raw_3a.bin
TUNING_BIN += s5k3p3sm_mipi_raw_shading.bin
TUNING_BIN += s5k3p3sm_mipi_raw_caf.bin
TUNING_BIN += s5k4h8yx_mipi_raw_tuning.bin
TUNING_BIN += imx230_mipi_raw_3a.bin
TUNING_BIN += imx230_mipi_raw_1280_3a.bin
TUNING_BIN += imx230_mipi_raw_shading.bin
TUNING_BIN += imx230_mipi_raw_caf.bin
TUNING_BIN += imx258_mipi_raw_tuning.bin
TUNING_BIN += ov13855_mipi_raw_3a.bin
TUNING_BIN += ov13855_mipi_raw_shading.bin
TUNING_BIN += ov13855_mipi_raw_caf.bin
TUNING_BIN += ov13870_mipi_raw_3a.bin
TUNING_BIN += ov13870_mipi_raw_shading.bin
TUNING_BIN += ov13870_mipi_raw_caf.bin
TUNING_BIN += ov2680_mipi_raw_tuning.bin
TUNING_BIN += s5k3l8xxm3_mipi_raw_tuning.bin
TUNING_BIN += s5k3l8xxm3_mipi_raw_cbc.bin
TUNING_BIN += s5k3l8xxm3_mipi_raw_pdotp.bin
TUNING_BIN += ov8856_mipi_raw_tuning.bin
TUNING_BIN += ov8856s_mipi_raw_tuning.bin
TUNING_BIN += sp2509_mipi_raw_tuning.bin
TUNING_BIN += ov8856s_mipi_raw_tuning.bin
TUNING_BIN += sp2509_mipi_raw_tuning.bin
TUNING_BIN += s5k3p8sm_mipi_raw_tuning.bin
TUNING_BIN += s5k3p8sm_mipi_raw_pdotp.bin
TUNING_BIN += s5k3p8sm_mipi_raw_cbc.bin
TUNING_BIN += s5k5e2ya_mipi_raw_tuning.bin

ALTEK_LIB := libalAWBLib \
	libalAELib \
	libalAFLib \
	libalFlickerLib \
	libspcaftrigger \
	libalRnBLV \
	libalPDAF \
	libalPDExtract \
	libalpdextract2 \
	libPDExtract \
	libalAFTrigger

ALTEK_FW := TBM_G2v1DDR.bin

SPRD_LIB := libcamoem
SPRD_LIB += libispalg

PRODUCT_PACKAGES += $(ALTEK_LIB)
PRODUCT_PACKAGES += $(ALTEK_FW)
PRODUCT_PACKAGES += $(TUNING_BIN)
PRODUCT_PACKAGES += $(SPRD_LIB)

# AL3200_FW
PRODUCT_PACKAGES += miniBoot.bin \
	TBM_D2.bin
PRODUCT_PACKAGES += libdepthengine
