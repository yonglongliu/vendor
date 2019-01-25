#!/system/bin/sh

a=`getprop zram.disksize 64`
b=`getprop sys.vm.swappiness 100`

kernel_version=$(uname -r)
support_lz4_kernel_ver="4"
kernel_main_ver=${kernel_version%%.*}

if [ $kernel_main_ver -ge $support_lz4_kernel_ver ]
then
    #use multistream
    num_cpus=$(cat /sys/devices/system/cpu/possible)
    dec_num_cpus=${num_cpus##*-}
    #if something goes wrong, assume we have 1
    [ "$dec_num_cpus" != 0 ] || dec_num_cpus=1

    echo $dec_num_cpus > /sys/block/zram0/max_comp_streams

    #select lz4 compression algorithm
    echo lz4 > /sys/block/zram0/comp_algorithm
fi

echo $(($a*1024*1024)) > /sys/block/zram0/disksize

#mknod /dev/block/zram0 b 253 0
mkswap /dev/block/zram0
swapon /dev/block/zram0
echo $(($b)) > /proc/sys/vm/swappiness



