#!/system/bin/sh

LOG_TAG="netbox.sh"

function exe_log()
{
	local cmd="$@"
	local emsg="$(eval $cmd 2>&1)"

	log -p d -t "$LOG_TAG" "$cmd ecode=$? $emsg"
}

function gro_switch()
{
	exe_log "echo $1 > /sys/module/seth/parameters/gro_enable"
}

if [ "$1" == "gro" ]; then
	if [ "$2" == "on" ]; then
		gro_switch 1;
	elif [ "$2" == "off" ]; then
		gro_switch 0;
	fi
fi
