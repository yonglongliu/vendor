#!/system/bin/sh

LOG_TAG="data_rps.sh"
rfc=4096
cc=4
function mlog()
{
	echo "$@"
	log -p d -t "$LOG_TAG" "$@"
}

function exe_log()
{
	mlog "$@";
	eval $@;
}

function rps_on()
{
	exe_log "chmod o+r /sys/devices/system/cpu/cpuhotplug/dynamic_load_disable"
	##set the all cpu core online
	exe_log "echo 1 >/sys/devices/system/cpu/cpuhotplug/dynamic_load_disable";
	sleep 1;
	##Enable RPS (Receive Packet Steering)
	((rsfe=$cc*$rfc));
	echo "$rsfe";
	exe_log sysctl -w net.core.rps_sock_flow_entries=$rsfe;
	for fileRps in $(ls /sys/class/net/seth_lte*/queues/rx-*/rps_cpus)
		do
			exe_log "echo f > $fileRps";
			retVal=$(cat $fileRps)
			mlog "the value of $fileRps is $retVal"
		done
	for fileRfc in $(ls /sys/class/net/seth_lte*/queues/rx-*/rps_flow_cnt)
		do
			exe_log "echo $rfc > $fileRfc";
			retVal=$(cat $fileRfc)
			mlog "the value of $fileRps is $retVal"
		done
}

function rps_off()
{
	exe_log "chmod o+r /sys/devices/system/cpu/cpuhotplug/dynamic_load_disable"
	exe_log "echo 0 >/sys/devices/system/cpu/cpuhotplug/dynamic_load_disable"
	exe_log sysctl -w net.core.rps_sock_flow_entries=0;
	for fileRps in $(ls /sys/class/net/seth_lte*/queues/rx-*/rps_cpus)
		do
			exe_log "echo 0 > $fileRps";
			retVal=$(cat $fileRps)
			mlog "the value of $fileRps is $retVal"
		done
	for fileRfc in $(ls /sys/class/net/seth_lte*/queues/rx-*/rps_flow_cnt)
		do
			exe_log "echo 0 > $fileRfc";
			retVal=$(cat $fileRfc)
			mlog "the value of $fileRps is $retVal"
		done
}

if [ "$1" = "on" ]; then
	rps_on;
elif [ "$1" = "off" ]; then
	rps_off;
fi