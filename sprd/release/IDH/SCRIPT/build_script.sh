#!/bin/bash
Conf="conf-sharkle.tar.gz"
Proprietories=$1
Project=`echo $Proprietories |awk -F '.zip' '{print$1}'|awk -F 'proprietories-' '{print$NF}'`
BASE_PATCH="`pwd`/.."
IMG_DIR="$BASE_PATCH/../../../.."

	echo "IMG_DIR=$IMG_DIR  Project=$Project   BASE_PATCH=$BASE_PATCH"
	read 

function BUILD(){
	#设置环境变量
	TMP_IOT_DIR="/tmp/iot-"$USER
	SR_CC="/usr/bin/gcc-4.8"
	SR_PP="/usr/bin/g++-4.8"
	DT_CC=${TMP_IOT_DIR}"/gcc"
	DT_PP=${TMP_IOT_DIR}"/g++"
	export PATH=${TMP_IOT_DIR}:$PATH
	export SHELL='/bin/bash'
	gcc -v && g++ -v

	echo "解压 Conf"
	if [ -n "$Conf" ] && [ -e $BASE_PATCH/$Conf ] ;then
		rm -rf $IMG_DIR/device
		tar -xf $BASE_PATCH/$Conf -C $IMG_DIR
	else
		echo "Conf 文件不存在！"
		exit 1
	fi
	
	echo "解压 Proprietories"
	if [ -n "$Proprietories" ] && [ -e $BASE_PATCH/$Proprietories ];then
		rm -rf $IMG_DIR/out
		unzip -o -d $IMG_DIR $BASE_PATCH/$Proprietories
	else
		echo "Proprietories 文件不存在！"
		exit 1
	fi

	cd $IMG_DIR
	source build/envsetup.sh
	lunch $Project
	make -j16 	#IDH_PROP_ZIP=$IMG_DIR/vendor/sprd/release/IDH/proprietories-${Project}.zip
}

function CONFIG(){		#生成配置文件
cd $BASE_PATCH/SCRIPT
chmod a+x *
product=`cd $IMG_DIR/out/target/product && ls -d sp* `
if [ -n "$product" ];then
	if [ "$Project" == "sp9820e_1h10_k_native-userdebug" ] || [ "$Project" == "sp9820e_1h10_k_native-user" ];then
		Modem_nv="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_Kois_builddir/sharkle_pubcp_Feature_Phone_Kois_nvitem.bin"
		Modem_dat="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SC9600_sharkle_pubcp_Feature_Phone_modem.dat"
		Modem_ldsp="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SharkLE_LTEA_DSP_evs_off.bin"
		Modem_gdsp="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SHARKLE1_DM_DSP.bin"
		Modem_cm="$BASE_PATCH/Modem/CP/sharkle_cm4_builddir/sharkle_cm4.bin"

		if [ -e "$IMG_DIR/out/target/product/$product/PRODUCT_SECURE_BOOT_SPRD" ];then
			if [ -d $IMG_DIR/sps.image ];then
				rm -rfv $IMG_DIR/sps.image
			fi
			mkdir -p $IMG_DIR/sps.image/$Project
			cp $IMG_DIR/out/target/product/$product/PRODUCT_SECURE_BOOT_SPRD $IMG_DIR/sps.image/$Project
			$BASE_PATCH/SCRIPT/sign_modem_image.sh $IMG_DIR/sps.image $Project $Modem_dat $Modem_ldsp $Modem_gdsp $Modem_cm
			#修改 bin 名称
			Modem_dat=$IMG_DIR/sps.image/$Project/${Modem_dat##*/} && Modem_dat=`echo $Modem_dat |sed 's/.bin/-sign.bin/'` && Modem_dat=`echo $Modem_dat |sed 's/.dat/-sign.dat/'`
			Modem_ldsp=$IMG_DIR/sps.image/$Project/${Modem_ldsp##*/} && Modem_ldsp=`echo $Modem_ldsp |sed 's/.bin/-sign.bin/'`
			Modem_gdsp=$IMG_DIR/sps.image/$Project/${Modem_gdsp##*/} && Modem_gdsp=`echo $Modem_gdsp |sed 's/.bin/-sign.bin/'`
			Modem_cm=$IMG_DIR/sps.image/$Project/${Modem_cm##*/} && Modem_cm=`echo $Modem_cm |sed 's/.bin/-sign.bin/'`
			if [ -e "$Modem_dat" ] && [ -e "$Modem_ldsp" ] && [ -e "$Modem_gdsp" ] && [ -e "$Modem_cm" ];then
				echo "Modem Sign PASS!"
			else
				echo "Modem Sign FAIL!"
				exit 1
			fi
		fi

		echo "[Setting]">config.ini
		echo "NandFlash=1">>config.ini
		echo "PAC_PRODUCT=$product">>config.ini
		echo "PAC_CONFILE=$IMG_DIR/out/target/product/$product/${product}.xml">>config.ini
		echo " ">>config.ini
		echo "[$product]">>config.ini
		echo "FDL=1@$IMG_DIR/out/target/product/$product/fdl1-sign.bin">>config.ini
		echo "FDL2=1@$IMG_DIR/out/target/product/$product/fdl2-sign.bin">>config.ini
		echo "NV_WLTE=1@$Modem_nv">>config.ini
		echo "ProdNV=1@$IMG_DIR/out/target/product/$product/prodnv.img">>config.ini
		echo "PhaseCheck=1@">>config.ini
		echo "EraseUBOOT=1@">>config.ini
		echo "SPLLoader=1@$IMG_DIR/out/target/product/$product/u-boot-spl-16k-sign.bin">>config.ini
		echo "Modem_WLTE=1@$Modem_dat">>config.ini
		echo "DSP_WLTE_LTE=1@$Modem_ldsp">>config.ini
		echo "DSP_WLTE_GGE=1@$Modem_gdsp">>config.ini
		echo "DFS=1@$Modem_cm">>config.ini
		echo "Modem_WCN=1@$BASE_PATCH/Modem/WCN/sharkle_cm4_builddir/PM_sharkle_cm4.bin">>config.ini
		echo "GPS_GL=1@$BASE_PATCH/Modem/GPS/gnssmodem.bin">>config.ini
		echo "GPS_BD=1@$BASE_PATCH/Modem/GPS/gnssbdmodem.bin">>config.ini
		echo "BOOT=1@$IMG_DIR/out/target/product/$product/boot-sign.img">>config.ini
		echo "Recovery=1@$IMG_DIR/out/target/product/$product/recovery-sign.img">>config.ini
		echo "System=1@$IMG_DIR/out/target/product/$product/system.img">>config.ini
		echo "UserData=1@$IMG_DIR/out/target/product/$product/userdata.img">>config.ini
		echo "BootLogo=1@$BASE_PATCH/SCRIPT/sprd_240320.bmp">>config.ini
		echo "Fastboot_Logo=1@$BASE_PATCH/SCRIPT/sprd_240320.bmp">>config.ini
		echo "Cache=1@$IMG_DIR/out/target/product/$product/cache.img">>config.ini
		echo "Trustos=1@$IMG_DIR/out/target/product/$product/tos-sign.bin">>config.ini
		echo "SML=1@$IMG_DIR/out/target/product/$product/sml-sign.bin">>config.ini
		echo "UBOOTLoader=1@$IMG_DIR/out/target/product/$product/u-boot-sign.bin">>config.ini
		
	elif [ "$Project" == "sp9820e_1h10_k_4m_native-userdebug" ] || [ "$Project" == "sp9820e_1h10_k_4m_native-user" ];then
		Modem_nv="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/sharkle_pubcp_Feature_Phone_nvitem.bin"
		Modem_dat="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SC9600_sharkle_pubcp_Feature_Phone_modem.dat"
		Modem_ldsp="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SharkLE_LTEA_DSP_evs_off.bin"
		Modem_gdsp="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SHARKLE1_DM_DSP.bin"
		Modem_cm="$BASE_PATCH/Modem/CP/sharkle_cm4_builddir/sharkle_cm4.bin"

		if [ -e "$IMG_DIR/out/target/product/$product/PRODUCT_SECURE_BOOT_SPRD" ];then
			if [ -d $IMG_DIR/sps.image ];then
				rm -rfv $IMG_DIR/sps.image
			fi
			mkdir -p $IMG_DIR/sps.image/$Project
			cp $IMG_DIR/out/target/product/$product/PRODUCT_SECURE_BOOT_SPRD $IMG_DIR/sps.image/$Project
			$BASE_PATCH/SCRIPT/sign_modem_image.sh $IMG_DIR/sps.image $Project $Modem_dat $Modem_ldsp $Modem_gdsp $Modem_cm
			#修改 bin 名称
			Modem_dat=$IMG_DIR/sps.image/$Project/${Modem_dat##*/} && Modem_dat=`echo $Modem_dat |sed 's/.bin/-sign.bin/'` && Modem_dat=`echo $Modem_dat |sed 's/.dat/-sign.dat/'`
			Modem_ldsp=$IMG_DIR/sps.image/$Project/${Modem_ldsp##*/} && Modem_ldsp=`echo $Modem_ldsp |sed 's/.bin/-sign.bin/'`
			Modem_gdsp=$IMG_DIR/sps.image/$Project/${Modem_gdsp##*/} && Modem_gdsp=`echo $Modem_gdsp |sed 's/.bin/-sign.bin/'`
			Modem_cm=$IMG_DIR/sps.image/$Project/${Modem_cm##*/} && Modem_cm=`echo $Modem_cm |sed 's/.bin/-sign.bin/'`
			if [ -e "$Modem_dat" ] && [ -e "$Modem_ldsp" ] && [ -e "$Modem_gdsp" ] && [ -e "$Modem_cm" ];then
				echo "Modem Sign PASS!"
			else
				echo "Modem Sign FAIL!"
				exit 1
			fi
		fi

		echo "[Setting]">config.ini
		echo "NandFlash=1">>config.ini
		echo "PAC_PRODUCT=$product">>config.ini
		echo "PAC_CONFILE=$IMG_DIR/out/target/product/$product/${product}.xml">>config.ini
		echo " ">>config.ini
		echo "[$product]">>config.ini
		echo "FDL=1@$IMG_DIR/out/target/product/$product/fdl1-sign.bin">>config.ini
		echo "FDL2=1@$IMG_DIR/out/target/product/$product/fdl2-sign.bin">>config.ini
		echo "NV_WLTE=1@$Modem_nv">>config.ini
		echo "ProdNV=1@$IMG_DIR/out/target/product/$product/prodnv.img">>config.ini
		echo "PhaseCheck=1@">>config.ini
		echo "EraseUBOOT=1@">>config.ini
		echo "SPLLoader=1@$IMG_DIR/out/target/product/$product/u-boot-spl-16k-sign.bin">>config.ini
		echo "Modem_WLTE=1@$Modem_dat">>config.ini
		echo "DSP_WLTE_LTE=1@$Modem_ldsp">>config.ini
		echo "DSP_WLTE_GGE=1@$Modem_gdsp">>config.ini
		echo "DFS=1@$Modem_cm">>config.ini
		echo "Modem_WCN=1@$BASE_PATCH/Modem/WCN/sharkle_cm4_builddir/PM_sharkle_cm4.bin">>config.ini
		echo "GPS_GL=1@$BASE_PATCH/Modem/GPS/gnssmodem.bin">>config.ini
		echo "GPS_BD=1@$BASE_PATCH/Modem/GPS/gnssbdmodem.bin">>config.ini
		echo "BOOT=1@$IMG_DIR/out/target/product/$product/boot.img">>config.ini
		echo "Recovery=1@$IMG_DIR/out/target/product/$product/recovery.img">>config.ini
		echo "System=1@$IMG_DIR/out/target/product/$product/system.img">>config.ini
		echo "UserData=1@$IMG_DIR/out/target/product/$product/userdata.img">>config.ini
		echo "BootLogo=1@$BASE_PATCH/SCRIPT/sprd_240320.bmp">>config.ini
		echo "Fastboot_Logo=1@$BASE_PATCH/SCRIPT/sprd_240320.bmp">>config.ini
		echo "Cache=1@$IMG_DIR/out/target/product/$product/cache.img">>config.ini
		echo "Trustos=1@$IMG_DIR/out/target/product/$product/tos-sign.bin">>config.ini
		echo "SML=1@$IMG_DIR/out/target/product/$product/sml-sign.bin">>config.ini
		echo "UBOOTLoader=1@$IMG_DIR/out/target/product/$product/u-boot-sign.bin">>config.ini
		
	elif [ "$Project" == "sp9820e_2h10_k_native-userdebug" ] || [ "$Project" == "sp9820e_2h10_k_native-user" ];then
		Modem_nv="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_Kois_builddir/sharkle_pubcp_Feature_Phone_Kois_nvitem.bin"
		Modem_dat="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SC9600_sharkle_pubcp_Feature_Phone_modem.dat"
		Modem_ldsp="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SharkLE_LTEA_DSP_evs_off.bin"
		Modem_gdsp="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SHARKLE1_DM_DSP.bin"
		Modem_cm="$BASE_PATCH/Modem/CP/sharkle_cm4_builddir/sharkle_cm4.bin"

		if [ -e "$IMG_DIR/out/target/product/$product/PRODUCT_SECURE_BOOT_SPRD" ];then
			if [ -d $IMG_DIR/sps.image ];then
				rm -rfv $IMG_DIR/sps.image
			fi
			mkdir -p $IMG_DIR/sps.image/$Project
			cp $IMG_DIR/out/target/product/$product/PRODUCT_SECURE_BOOT_SPRD $IMG_DIR/sps.image/$Project
			$BASE_PATCH/SCRIPT/sign_modem_image.sh $IMG_DIR/sps.image $Project $Modem_dat $Modem_ldsp $Modem_gdsp $Modem_cm
			#修改 bin 名称
			Modem_dat=$IMG_DIR/sps.image/$Project/${Modem_dat##*/} && Modem_dat=`echo $Modem_dat |sed 's/.bin/-sign.bin/'` && Modem_dat=`echo $Modem_dat |sed 's/.dat/-sign.dat/'`
			Modem_ldsp=$IMG_DIR/sps.image/$Project/${Modem_ldsp##*/} && Modem_ldsp=`echo $Modem_ldsp |sed 's/.bin/-sign.bin/'`
			Modem_gdsp=$IMG_DIR/sps.image/$Project/${Modem_gdsp##*/} && Modem_gdsp=`echo $Modem_gdsp |sed 's/.bin/-sign.bin/'`
			Modem_cm=$IMG_DIR/sps.image/$Project/${Modem_cm##*/} && Modem_cm=`echo $Modem_cm |sed 's/.bin/-sign.bin/'`
			if [ -e "$Modem_dat" ] && [ -e "$Modem_ldsp" ] && [ -e "$Modem_gdsp" ] && [ -e "$Modem_cm" ];then
				echo "Modem Sign PASS!"
			else
				echo "Modem Sign FAIL!"
				exit 1
			fi
		fi

		echo "[Setting]">config.ini
		echo "NandFlash=1">>config.ini
		echo "PAC_PRODUCT=$product">>config.ini
		echo "PAC_CONFILE=$IMG_DIR/out/target/product/$product/${product}.xml">>config.ini
		echo " ">>config.ini
		echo "[$product]">>config.ini
		echo "FDL=1@$IMG_DIR/out/target/product/$product/fdl1-sign.bin">>config.ini
		echo "FDL2=1@$IMG_DIR/out/target/product/$product/fdl2-sign.bin">>config.ini
		echo "NV_WLTE=1@$Modem_nv">>config.ini
		echo "ProdNV=1@$IMG_DIR/out/target/product/$product/prodnv_b256k_p4k.img">>config.ini
		echo "PhaseCheck=1@">>config.ini
		echo "EraseUBOOT=1@">>config.ini
		echo "SPLLoader=1@$IMG_DIR/out/target/product/$product/u-boot-spl-16k-sign.bin">>config.ini
		echo "Modem_WLTE=1@$Modem_dat">>config.ini
		echo "DSP_WLTE_LTE=1@$Modem_ldsp">>config.ini
		echo "DSP_WLTE_GGE=1@$Modem_gdsp">>config.ini
		echo "DFS=1@$Modem_cm">>config.ini
		echo "Modem_WCN=1@$BASE_PATCH/Modem/WCN/sharkle_cm4_builddir/PM_sharkle_cm4.bin">>config.ini
		echo "GPS_GL=1@$BASE_PATCH/Modem/GPS/gnssmodem.bin">>config.ini
		echo "GPS_BD=1@$BASE_PATCH/Modem/GPS/gnssbdmodem.bin">>config.ini
		echo "BOOT=1@$IMG_DIR/out/target/product/$product/boot.img">>config.ini
		echo "Recovery=1@$IMG_DIR/out/target/product/$product/recovery.img">>config.ini
		echo "System=1@$IMG_DIR/out/target/product/$product/system_b256k_p4k.img">>config.ini
		echo "UserData=1@$IMG_DIR/out/target/product/$product/userdata_b256k_p4k.img">>config.ini
		echo "BootLogo=1@$BASE_PATCH/SCRIPT/sprd_240320.bmp">>config.ini
		echo "Fastboot_Logo=1@$BASE_PATCH/SCRIPT/sprd_240320.bmp">>config.ini
		echo "Cache=1@$IMG_DIR/out/target/product/$product/cache_b256k_p4k.img">>config.ini
		echo "Trustos=1@$IMG_DIR/out/target/product/$product/tos-sign.bin">>config.ini
		echo "SML=1@$IMG_DIR/out/target/product/$product/sml-sign.bin">>config.ini
		echo "UBOOTLoader=1@$IMG_DIR/out/target/product/$product/u-boot-sign.bin">>config.ini

	elif [ "$Project" == "sp9820e_13c10_k_native-userdebug" ] || [ "$Project" == "sp9820e_13c10_k_native-user" ] || [ "$Project" == "sp9820e_2c10_k_native-userdebug" ] || [ "$Project" == "sp9820e_2c10_k_native-user" ] || [ "$Project" == "sp9820e_5c10_k_native-userdebug" ] || [ "$Project" == "sp9820e_5c10_k_native-user" ];then
		Modem_nv="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_LB4020E_builddir/sharkle_pubcp_Feature_Phone_LB4020E_nvitem.bin"
		Modem_dat="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SC9600_sharkle_pubcp_Feature_Phone_modem.dat"
		Modem_ldsp="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SharkLE_LTEA_DSP_evs_off.bin"
		Modem_gdsp="$BASE_PATCH/Modem/CP/sharkle_pubcp_Feature_Phone_builddir/SHARKLE1_DM_DSP.bin"
		Modem_cm="$BASE_PATCH/Modem/CP/sharkle_cm4_builddir/sharkle_cm4.bin"

		if [ -e "$IMG_DIR/out/target/product/$product/PRODUCT_SECURE_BOOT_SPRD" ];then
			if [ -d $IMG_DIR/sps.image ];then
				rm -rfv $IMG_DIR/sps.image
			fi
			mkdir -p $IMG_DIR/sps.image/$Project
			cp $IMG_DIR/out/target/product/$product/PRODUCT_SECURE_BOOT_SPRD $IMG_DIR/sps.image/$Project
			$BASE_PATCH/SCRIPT/sign_modem_image.sh $IMG_DIR/sps.image $Project $Modem_dat $Modem_ldsp $Modem_gdsp $Modem_cm
			#修改 bin 名称
			Modem_dat=$IMG_DIR/sps.image/$Project/${Modem_dat##*/} && Modem_dat=`echo $Modem_dat |sed 's/.bin/-sign.bin/'` && Modem_dat=`echo $Modem_dat |sed 's/.dat/-sign.dat/'`
			Modem_ldsp=$IMG_DIR/sps.image/$Project/${Modem_ldsp##*/} && Modem_ldsp=`echo $Modem_ldsp |sed 's/.bin/-sign.bin/'`
			Modem_gdsp=$IMG_DIR/sps.image/$Project/${Modem_gdsp##*/} && Modem_gdsp=`echo $Modem_gdsp |sed 's/.bin/-sign.bin/'`
			Modem_cm=$IMG_DIR/sps.image/$Project/${Modem_cm##*/} && Modem_cm=`echo $Modem_cm |sed 's/.bin/-sign.bin/'`
			if [ -e "$Modem_dat" ] && [ -e "$Modem_ldsp" ] && [ -e "$Modem_gdsp" ] && [ -e "$Modem_cm" ];then
				echo "Modem Sign PASS!"
			else
				echo "Modem Sign FAIL!"
				exit 1
			fi
		fi

		echo "[Setting]">config.ini
		echo "NandFlash=1">>config.ini
		echo "PAC_PRODUCT=$product">>config.ini
		echo "PAC_CONFILE=$IMG_DIR/out/target/product/$product/${product}.xml">>config.ini
		echo " ">>config.ini
		echo "[$product]">>config.ini
		echo "FDL=1@$IMG_DIR/out/target/product/$product/fdl1-sign.bin">>config.ini
		echo "FDL2=1@$IMG_DIR/out/target/product/$product/fdl2-sign.bin">>config.ini
		echo "NV_WLTE=1@$Modem_nv">>config.ini
		echo "ProdNV=1@$IMG_DIR/out/target/product/$product/prodnv.img">>config.ini
		echo "PhaseCheck=1@">>config.ini
		echo "EraseUBOOT=1@">>config.ini
		echo "SPLLoader=1@$IMG_DIR/out/target/product/$product/u-boot-spl-16k-sign.bin">>config.ini
		echo "Modem_WLTE=1@$Modem_dat">>config.ini
		echo "DSP_WLTE_LTE=1@$Modem_ldsp">>config.ini
		echo "DSP_WLTE_GGE=1@$Modem_gdsp">>config.ini
		echo "DFS=1@$Modem_cm">>config.ini
		echo "Modem_WCN=1@$BASE_PATCH/Modem/WCN/sharkle_cm4_builddir/PM_sharkle_cm4.bin">>config.ini
		echo "GPS_GL=1@$BASE_PATCH/Modem/GPS/gnssmodem.bin">>config.ini
		echo "GPS_BD=1@$BASE_PATCH/Modem/GPS/gnssbdmodem.bin">>config.ini
		echo "BOOT=1@$IMG_DIR/out/target/product/$product/boot.img">>config.ini
		echo "Recovery=1@$IMG_DIR/out/target/product/$product/recovery.img">>config.ini
		echo "System=1@$IMG_DIR/out/target/product/$product/system.img">>config.ini
		echo "UserData=1@$IMG_DIR/out/target/product/$product/userdata.img">>config.ini
		echo "BootLogo=1@$BASE_PATCH/SCRIPT/sprd_240320.bmp">>config.ini
		echo "Fastboot_Logo=1@$BASE_PATCH/SCRIPT/sprd_240320.bmp">>config.ini
		echo "Cache=1@$IMG_DIR/out/target/product/$product/cache.img">>config.ini
		echo "Trustos=1@$IMG_DIR/out/target/product/$product/tos-sign.bin">>config.ini
		echo "SML=1@$IMG_DIR/out/target/product/$product/sml-sign.bin">>config.ini
		echo "UBOOTLoader=1@$IMG_DIR/out/target/product/$product/u-boot-sign.bin">>config.ini
	else
		echo "$Project CONFIG FAIL!"
		exit 1
	fi

else
	echo -e "\n $IMG_DIR/out/target/product/$product check fail! \n"
	exit 1
fi
}

function PAC(){		#生成 pac 包
	if [ -d $BASE_PATCH/Modem ];then
		rm -rf $BASE_PATCH/Modem
	fi

	mkdir -p $BASE_PATCH/Modem/CP
	Modem_CP=`ls $BASE_PATCH/FM_BASE_17B_Release_W*rar`
	unrar x $Modem_CP -C $BASE_PATCH/Modem/CP
	mv $BASE_PATCH/Modem/CP/FM_BASE_17B_Release_W*/*_builddir $BASE_PATCH/Modem/CP

	mkdir -p $BASE_PATCH/Modem/WCN
	Modem_WCN=`ls $BASE_PATCH/WCN_Marlin2_17A_W*rar`
	unrar x $Modem_WCN -C $BASE_PATCH/Modem/WCN
	mv $BASE_PATCH/Modem/WCN/WCN_Marlin2_17A_W*/*_builddir $BASE_PATCH/Modem/WCN

	mkdir -p $BASE_PATCH/Modem/GPS
	Modem_GPS=`ls $BASE_PATCH/GPS*rar`
	unrar x $Modem_GPS -C $BASE_PATCH/Modem/GPS

	CONFIG
	perl sprd_pac.pl "$Project.pac"  sprd6.0 config.ini $product
}

BUILD
PAC
