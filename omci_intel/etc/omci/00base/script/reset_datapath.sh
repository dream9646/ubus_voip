#!/bin/sh
for i in $(seq 1 10); do
	echo "reset datapath $1" > /dev/console
	echo 1 > /proc/rg/reset_sw_recovery_hwnat
	# use ping slave to judge if the reset_datapath has been issued successfully
	ping -q 10.253.253.2 -c1 -w1 2>/dev/null >/dev/null && break
done
