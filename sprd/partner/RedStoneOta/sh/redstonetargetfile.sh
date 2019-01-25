#!/bin/bash
#$1 OTATOOLS
#$2 PRODUCT_OUT
#$3 ziproot path
#$4 
#$5
#echo $PWD
echo $1
echo $2
echo $3

ROOTPATH="rs_target_files"
RELEASETOOLS="build/tools/releasetools/"
MTK_RELEASETOOLS="device/mediatek/build/releasetools/"

MISC_INFO="$3/META/misc_info.txt"
#TOOL_EXTENSIONS="$(grep "tool_extensions" $MISC_INFO | awk -F '=' '{print $2}')"
KEY_CERT_PAIR="$(grep "default_system_dev_certificate" $MISC_INFO | awk -F '=' '{print $2}')"
VERITY_KEY_CERT_PAIR="$(grep "verity_key" $MISC_INFO | awk -F '=' '{print $2}')"
MODEM_UPDATE_CONFIG_PAIR="$(grep "modem_update_config_file" $MISC_INFO | awk -F '=' '{print $2}')"


mkdir -p $ROOTPATH
echo "Copy OTA tools:" 
cat $1|awk '{for(i=1;i<=NF;i++) print "cp -u --parent " $i " '$ROOTPATH'"}'|bash

echo "Copy releasetools"
#cp -a --parent build/target/product/security/  $ROOTPATH
cp -a --parent $MTK_RELEASETOOLS $ROOTPATH
cp -ur --parent $RELEASETOOLS $ROOTPATH

echo "Copy sprd modem_update_config_file"
cp -ur --parent $MODEM_UPDATE_CONFIG_PAIR $ROOTPATH

echo "Copy KEY_CERT_PAIR & VERITY_KEY_CERT_PAIR" 
cp -ur --parent $KEY_CERT_PAIR".x509.pem" $ROOTPATH
cp -ur --parent $KEY_CERT_PAIR".pk8" $ROOTPATH
cp -ur --parent $VERITY_KEY_CERT_PAIR".x509.pem" $ROOTPATH
cp -ur --parent $VERITY_KEY_CERT_PAIR".pk8" $ROOTPATH
cp -ur --parent $VERITY_KEY_CERT_PAIR"_key" $ROOTPATH

echo "Copy tool_extensions"
mkdir -p $ROOTPATH/system/extras/verity/
#cp -ur --parent $TOOL_EXTENSIONS $ROOTPATH
cp -ur system/extras/verity/build_verity_metadata.py $ROOTPATH/system/extras/verity/
cp -ur out/host/linux-x86/framework/VeritySigner.jar $ROOTPATH/out/host/linux-x86/framework/VeritySigner.jar

echo "Copy ota_scatter.txt"
cp -ur --parent $2/ota_scatter.txt $ROOTPATH

echo "Copy target_files"
cp -u $3.zip  $ROOTPATH/redstone_target_files.zip

echo "Copy build.prop"
cp -u $2/system/build.prop $ROOTPATH/build.prop

echo "Copy lk.bin"
cp -u $2/lk.bin $ROOTPATH/lk.bin

echo "Copy logo.bin"
cp -u $2/logo.bin $ROOTPATH/logo.bin

############################################################
#Generate redstone_fota_info.xml
XML_ROOT="$ROOTPATH/redstone_fota_info.xml"

function getValue()
{
    ret=$(grep "$1=" "$ROOTPATH/build.prop" | awk -F '=' '{print $2}');
    if [ -n "$ret" ] ; then
        echo $ret;
    else
        echo "none";
    fi

}

echo "<?xml version='1.0' encoding='utf-8' standalone='yes' ?>"> $XML_ROOT
echo "<FOTA>">>$XML_ROOT
echo "<Item name=\"ro.redstone.platform\" value=\"$(getValue ro.redstone.platform)\"></Item>">>$XML_ROOT
echo "<Item name=\"ro.redstone.brand\" value=\"$(getValue ro.redstone.brand)\"></Item>">>$XML_ROOT
echo "<Item name=\"ro.redstone.model\" value=\"$(getValue ro.redstone.model)\"></Item>">>$XML_ROOT
echo "<Item name=\"ro.redstone.version\" value=\"$(getValue ro.redstone.version)\"></Item>">>$XML_ROOT
echo "<Item name=\"ro.redstone.channelid\" value=\"$(getValue ro.redstone.channelid)\"></Item>">>$XML_ROOT
echo "<Item name=\"ro.redstone.appid\" value=\"$(getValue ro.redstone.appid)\"></Item>">>$XML_ROOT
echo "<Item name=\"ro.build.date.utc\" value=\"$(getValue ro.build.date.utc)\"></Item>">>$XML_ROOT
echo "</FOTA>">>$XML_ROOT

##################################################################################################

PACKAGE_NAME="redstone_$(echo $(getValue ro.redstone.version)|sed "s/ /_/g")_targetfile"_$(getValue ro.build.date.utc)

if [ -f $2/$PACKAGE_NAME.zip ]; then
echo "delete exist file:$2/$PACKAGE_NAME.zip"
rm -f $2/$PACKAGE_NAME.zip
fi

cd $ROOTPATH
zip -rq $ROOTPATH.zip ./
cd ..
mv $ROOTPATH/$ROOTPATH.zip $2/$PACKAGE_NAME.zip
rm -rf $ROOTPATH


