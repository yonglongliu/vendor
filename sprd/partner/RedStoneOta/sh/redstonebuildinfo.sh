# 根据项目配置以下内容
LOCAL_PROJECT_BRAND="ro.product.brand"
LOCAL_PROJECT_MODEL="ro.product.model"
LOCAL_PROJECT_VERSION="ro.build.display.id"
################################################
LOCAL_PROJECT_PLATFORM="ro.board.platform"
#echo "$2" REDSTONE_FOTA_APK_KEY
#echo "$3" REDSTONE_FOTA_APK_CHANNELID
LOCAL_PROJECT_APPID="$2"
LOCAL_PROJECT_CHANNELID="$3"


echo ""
echo "# start redstone OTA properties"
echo "$(grep "$LOCAL_PROJECT_PLATFORM" "$1" | sed "s/$LOCAL_PROJECT_PLATFORM/ro.redstone.platform/" )"
echo "$(grep "$LOCAL_PROJECT_BRAND" "$1" | sed "s/$LOCAL_PROJECT_BRAND/ro.redstone.brand/" )"
echo "$(grep "$LOCAL_PROJECT_MODEL" "$1" | sed "s/$LOCAL_PROJECT_MODEL/ro.redstone.model/" )"
#echo "$(grep "$LOCAL_PROJECT_VERSION" "$1" | sed "s/$LOCAL_PROJECT_VERSION/ro.redstone.version/" )"_`date +%s`
echo "$(grep "$LOCAL_PROJECT_VERSION" "$1" | sed "s/$LOCAL_PROJECT_VERSION/ro.redstone.version/" )"
if [ -n "$LOCAL_PROJECT_APPID" -a "$LOCAL_PROJECT_APPID" != "none" ]; then
echo "ro.redstone.appid=$LOCAL_PROJECT_APPID"
fi
if [ -n "$LOCAL_PROJECT_CHANNELID" -a "$LOCAL_PROJECT_CHANNELID" != "none" ]; then 
echo "ro.redstone.channelid=$LOCAL_PROJECT_CHANNELID"
fi
echo "# end redstone OTA properties"
