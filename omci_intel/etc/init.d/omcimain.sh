#!/bin/sh /etc/rc.common

START=85
USE_PROCD=1
OMCID_BIN=/usr/bin/omcimain


if [ -f $IPKG_INSTROOT/lib/pon.sh ]; then
	. $IPKG_INSTROOT/lib/pon.sh
fi


wait_for_jffs()
{
	while ! grep overlayfs:/overlay /proc/self/mounts >/dev/null
	do
		sleep 1
	done
}

is_flash_boot()
{
	grep overlayfs /proc/self/mounts >/dev/null
}

omci_hw_mac_get() {
	local mac_offset=1
	echo $1 | awk -v offs=$mac_offset 'BEGIN{FS=":"} {printf "%s:%s:%s:%s:%02x:%s\n", $1,$2,$3,$4,("0x"$5)+offs,$6}'
}

start_service() {
	local omcc_netdev

	config_load omci
	config_get omcc_netdev "default" "omcc_netdev"
	if [ -e /sys/class/net/${omcc_netdev}/address ]; then
		omcc_mac="$(cat /sys/class/net/${omcc_netdev}/address)"
		omcc_devmac="$(omci_hw_mac_get $omcc_mac)"
		omcc_args="-I ${omcc_mac} -M ${omcc_devmac} "
	fi

	${OMCID_BIN} ${omcc_args} &
}

start() {
	(
		# Wait until overlay is mounted.
		# This can take some time on first boot.
		is_flash_boot && wait_for_jffs

		rc_procd start_service "$@"
	) &
}
