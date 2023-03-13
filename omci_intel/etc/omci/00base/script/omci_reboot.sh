#!/bin/sh -x
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

on_sighup() {                                                                   
        echo "skip signal" > /dev/null
}                                                                               

trap on_sighup SIGTERM
trap on_sighup SIGQUIT
sync; sync
cat /dev/mtd/0 > /dev/null

/sbin/reboot &
exit 0

# script below is for 2310, they are not recessary for 2510
# so we reboot directly without killing process and umounting flash

# kill housekeeper code first
for p in looprun ; do
	pids=`pidof $p` && echo "Stop [$p] $pids" && kill -SIGKILL $pids
done

ps > /var/run/ps.$$
pids=`cat /var/run/ps.$$ | grep -v "\[" |grep -v init |grep -v PID |grep -v ash |cut -c1-5`
rm /var/run/ps.$$

kill -SIGTERM $pids

# umount possible fs, especially nfs
umount -a 2>/dev/null

# rmmod *
#for m in `lsmod|cut -d' ' -f1|grep -v Module`; do
#	rmmod $m && echo Remove module [$m]
#done

# down all interface
#for i in `ifconfig -a|cut -c1-8`; do
#	ifconfig $i down && echo Shutdown interface [$i]
#done

kill -SIGQUIT $pids > /dev/console

echo Umount jffs2 file system device.
cd /
umount /etc > /dev/console 2>&1
mount  > /dev/console

sync; sync
cat /dev/mtd/0 > /dev/null

/sbin/reboot &
