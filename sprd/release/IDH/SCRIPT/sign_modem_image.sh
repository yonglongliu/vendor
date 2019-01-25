#!/bin/bash
IMG_DIR=$1
PROJ=$2
MODEM_DFS=$3
MODEM_MODEM_T=$4
MODEM_DSP_L=$5
MODEM_DSP_T=${6}
MODEM_WARM_W=${7}

STOOL_PATH=$IMG_DIR/../out/host/linux-x86/bin
SPRD_CONFIG_FILE=$IMG_DIR/../vendor/sprd/proprietories-source/packimage_scripts/signimage/sprd/config
SANSA_CONFIG_PATH=$IMG_DIR/../vendor/sprd/proprietories-source/packimage_scripts/signimage/sansa/config
SANSA_OUTPUT_PATH=$IMG_DIR/../vendor/sprd/proprietories-source/packimage_scripts/signimage/sansa/output
SANSA_PYPATH=$IMG_DIR/../vendor/sprd/proprietories-source/packimage_scripts/signimage/sansa/python
SPRD_SECURE_FILE=$IMG_DIR/$PROJ/PRODUCT_SECURE_BOOT_SPRD

echo "STOOL_PATH =$STOOL_PATH   SPRD_SECURE_FILE=$SPRD_SECURE_FILE"
#read

doModemCopy()
{
	echo -e "\033[31m entry doModemCopy \033[0m"
	echo ${IMG_DIR}/${PROJ}
	cp $MODEM_MODEM_T ${IMG_DIR}/${PROJ}/
	echo $MODEM_MODEM_T
	cp $MODEM_DSP_T ${IMG_DIR}/${PROJ}/
	echo $MODEM_DSP_T
	cp $MODEM_DSP_L ${IMG_DIR}/${PROJ}/
	echo $MODEM_DSP_L
	cp $MODEM_WARM_W ${IMG_DIR}/${PROJ}/
	echo $MODEM_WARM_W
	cp $MODEM_DFS ${IMG_DIR}/${PROJ}/
	echo $MODEM_DFS
	echo -e "\033[31m now list img_dir/proj docment \033[0m"
	ls ${IMG_DIR}/${PROJ}
	echo -e "\033[31m out doModemCopy \033[0m"
}

getImageName()
{
    local imagename=$1
    echo ${imagename##*/}
}

getSignImageName()
{
    local temp1=$(getImageName $1)
    local left=${temp1%.*}
    local temp2=$(getImageName $1)
    local right=${temp2##*.}
    local signname=${left}"-sign."${right}
    echo $signname
}

getRawSignImageName()
{
    local temp1=$1
    local left=${temp1%.*}
    local temp2=$1
    local right=${temp2##*.}
    local signname=${left}"-sign."${right}
    echo $signname
}

getLogName()
{
    local temp=$(getImageName $1)
    local left=${temp%.*}
    local name=$SANSA_OUTPUT_PATH/${left}".log"
    echo $name
}

MODEM_LTE=${IMG_DIR}/${PROJ}/$(getImageName $MODEM_MODEM_T)
DSP_LTE_TG=${IMG_DIR}/${PROJ}/$(getImageName $MODEM_DSP_L)
DSP_LTE_LDSP=${IMG_DIR}/${PROJ}/$(getImageName $MODEM_DSP_T)
DSP_LTE_AG=${IMG_DIR}/${PROJ}/$(getImageName $MODEM_WARM_W)
DFS=${IMG_DIR}/${PROJ}/$(getImageName $MODEM_DFS)

doModemInsertHeader()
{
    echo "doModemInsertHeader enter"
    for loop in $@
    do
        if [ -f $loop ] ; then
            $STOOL_PATH/imgheaderinsert $loop 0
        else
            echo "#### no $loop,please check ####"
        fi
    done

    echo "doModemInsertHeader leave. now list"
	  ls ${IMG_DIR}/${PROJ}
    echo "list end"
}

makeModemCntCert()
{
    echo
    echo "makeModemCntCert: $1 "

    #local CNTCFG=${SANSA_CONFIG_PATH}/certcnt_modem.cfg
    local CNTCFG=${SANSA_CONFIG_PATH}/certcnt_3.cfg
    local SW_TBL=${SANSA_CONFIG_PATH}/SW.tbl

    sed -i "s#.* .* .*#$1 0xFFFFFFFFFFFFFFFF 0x200#" $SW_TBL

    if [ -f $1 ] ; then
        cd $IMG_DIR/..
        echo "switch to: `pwd`"
        echo "sign......"
        python3 $SANSA_PYPATH/bin/cert_sb_content_util.py $CNTCFG $(getLogName $1)
        echo "back to:"
        cd -
        echo
    else
        echo "#### no $1,please check ####"
    fi
}

genModemCntCfgFile()
{
    local CNTCFG=${SANSA_CONFIG_PATH}/certcnt_modem.cfg
    local KEY=${SANSA_CONFIG_PATH}/key_3.pem
    local KEYPWD=${SANSA_CONFIG_PATH}/pass_3.txt
    local SWTBL=${SANSA_CONFIG_PATH}/SW.tbl
    local NVID=2
    local NVVAL=1
    local CERTPKG=${SANSA_OUTPUT_PATH}/certcnt.bin

    echo "enter genModemCntCfgFile "

    if [ -f "$CNTCFG" ]; then
        rm -rf $CNTCFG
    fi

    touch ${CNTCFG}
    echo "[CNT-CFG]" >> ${CNTCFG}
    echo "cert-keypair = ${KEY}" >> ${CNTCFG}
    echo "cert-keypair-pwd = ${KEYPWD}" >> ${CNTCFG}
    echo "images-table = ${SWTBL}" >> ${CNTCFG}
    echo "nvcounter-id = ${NVID}" >> ${CNTCFG}
    echo "nvcounter-val = ${NVVAL}" >> ${CNTCFG}
    echo "cert-pkg = ${CERTPKG}" >> ${CNTCFG}
}

doSansaModemSignImage()
{
    echo "doSansaModemSignImage: $@ "
    #genModemCntCfgFile

    for loop in $@
    do
        if [ -f $loop ] ; then
            echo "call splitimg"
            $STOOL_PATH/splitimg $loop
            makeModemCntCert $loop
            $STOOL_PATH/signimage $SANSA_OUTPUT_PATH $(getRawSignImageName $loop) 1 0
        else
            echo "#### no $loop,please check ####"
        fi
    done
    echo "doSansaModemSignImage leave. now list"
	  ls ${IMG_DIR}/${PROJ}
    echo "list end"
}

doSprdModemSignImage()
{
    for loop in $@
    do
        if [ -f $loop ] ; then
	    $STOOL_PATH/sprd_sign $(getRawSignImageName $loop) $SPRD_CONFIG_FILE
        else
            echo "#### no $loop,please check ####"
        fi
    done
}

doModemSignImage()
{
    echo "doModemSignImage  proj = $PROJ"
    echo "IMG_DIR = $IMG_DIR"
    if (echo $PROJ | grep "spsec") 1>/dev/null 2>&1 ; then
        echo "found spsec"
        doModemInsertHeader $@
        doSprdModemSignImage $@
    elif [ -f "$SPRD_SECURE_FILE" ]; then
        echo "found sprd_secure_file"
        doModemInsertHeader $@
        doSprdModemSignImage $@
    elif (echo $PROJ | grep "dxsec") 1>/dev/null 2>&1 ; then
            echo "found dxsec"
            doModemInsertHeader $@
            doSansaModemSignImage $@
    elif (echo $PROJ | grep "P953F10_RU") 1>/dev/null 2>&1 ; then
            echo "found dxsec"
            doModemInsertHeader $@
            doSprdModemSignImage $@
 
    else
        echo "not secureboot proj, no need to sign!"
    fi
}

doModemPackImage()
{
    doModemCopy
#    doModemInsertHeader $@
    doModemSignImage $@
}

doModemPackImage $MODEM_LTE $DSP_LTE_TG $DSP_LTE_LDSP $DSP_LTE_AG $DFS
