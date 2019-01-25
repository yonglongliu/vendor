#! /bin/bash

IDH_BUILD_SYSTEM=vendor/sprd/build/buildidh
IDH_CODE_SCRIPT=$IDH_BUILD_SYSTEM/makeidh.pl

target_arch=$(cd $ANDROID_BUILD_TOP; CALLED_FROM_SETUP=true BUILD_SYSTEM=build/core make --no-print-directory -f build/core/config.mk dumpvar-TARGET_ARCH)

if [ "$target_arch" != "x86_64" ] ; then
  echo "[NOTRELEASE_LIST] not include x86_64"
  NOTRELEASE_LIST=`repo forall --group=idh,idhdel,sprd_intel -c 'echo $REPO_PATH,'`
else
  echo "[NOTRELEASE_LIST] include x86_64"
  NOTRELEASE_LIST=`repo forall --group=idh,idhdel -c 'echo $REPO_PATH,'`
fi

NOTRELEASES=`echo $NOTRELEASE_LIST | sed 's/[[:space:]]//g'`

#EXCLUDE_RELEASE is a white list
EXCLUDE_RELEASE=$1

#make idh.code.tgz and conf-$plat.tar.gz
if [ ! -f sps.image/idh.code.tgz ]; then
/usr/bin/perl $IDH_CODE_SCRIPT `pwd` "$NOTRELEASES" "$EXCLUDE_RELEASE"
fi
