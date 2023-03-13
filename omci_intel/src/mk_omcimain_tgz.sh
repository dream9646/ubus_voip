#!/bin/sh

# uncomment to build omci for x86
#TOOLPREFIX=/usr/bin/

# uncomment to build omci for brcm63xx
#CONFIG_TARGET_BRCM63XX=y

# uncomment to enable bind_mac package
#OMCI_BM_ENABLE=y

########################################################################

SCRIPT_NAME=$0
CURDIR=$(cd $(dirname $SCRIPT_NAME); pwd)
[ "$1" = "clean" ] && CLEAN=y

platform_prx321() {
	echo "platform prx321"

	# override TOOLPREFIX only if it is empty
	[ -z "$TOOLPREFIX" ] && TOOLPREFIX=$toolprefix
	export TOOLPREFIX

	echo "$SCRIPT_NAME  KERNEL_DIR=$KERNEL_DIR"
	echo "$SCRIPT_NAME  TOOLPREFIX=$TOOLPREFIX"
	#echo "$SCRIPT_NAME  CHIPSET=$CHIPSET"

	echo "# setting of $(basename $SCRIPT_NAME)  $(date)
TOOLPREFIX=$TOOLPREFIX
CONFIG_TARGET_PRX321=$CONFIG_TARGET_PRX321
OMCI_BM_ENABLE=$OMCI_BM_ENABLE
KERNEL_DIR=$KERNEL_DIR
" > $CURDIR/mk_omcimain_tgz.sh.config
}

platform_prx120() {
	echo "platform prx120"

	# override TOOLPREFIX only if it is empty
	[ -z "$TOOLPREFIX" ] && TOOLPREFIX=$toolprefix
	export TOOLPREFIX

	echo "$SCRIPT_NAME  KERNEL_DIR=$KERNEL_DIR"
	echo "$SCRIPT_NAME  TOOLPREFIX=$TOOLPREFIX"
	#echo "$SCRIPT_NAME  CHIPSET=$CHIPSET"

	echo "# setting of $(basename $SCRIPT_NAME)  $(date)
TOOLPREFIX=$TOOLPREFIX
CONFIG_TARGET_PRX120=$CONFIG_TARGET_PRX120
OMCI_BM_ENABLE=$OMCI_BM_ENABLE
KERNEL_DIR=$KERNEL_DIR
" > $CURDIR/mk_omcimain_tgz.sh.config
}

platform_prx126() {
	echo "platform prx126"
	# for rtk platform, the script determines KERNEL_DIR, PLATFORM_DIR, TOOLPREFIX for target Makefile

	# uncomment either one to force build base on specific kernel
	#KERNEL_DIR=`cd ../../../linux-2.6.30.9; pwd`
	#KERNEL_DIR=`cd ../../../linux-3.18.x; pwd`
	#KERNEL_DIR=`cd ../../linux-4.4.x; pwd`

	# guess KERNEL_DIR location
	#if [ -z "$KERNEL_DIR" ]; then
	#	if [ -d ../../../linux-2.6.30.9 ]; then
	#		KERNEL_DIR=`cd ../../../linux-2.6.30.9; pwd`
	#	elif [ -d ../../../linux-3.18.x ]; then
	#		KERNEL_DIR=`cd ../../../linux-3.18.x; pwd`
	#	elif [ -d ../../linux-4.4.x ]; then
	#		KERNEL_DIR=`cd ../../linux-4.4.x; pwd`
	#	else
	#		echo "KERNEL_DIR not found?"
	#		exit 1
	#	fi
	#fi
	#export KERNEL_DIR

	#toolprefix=/opt/ipq/toolchain-arm_cortex-a7_gcc-5.2.0_musl-1.1.16_eabi/bin/arm-openwrt-linux-muslgnueabi-
	#toolprefix=/opt/ipq/toolchain-aarch64_cortex-a53_gcc-5.2.0_musl-1.1.16/bin/arm-openwrt-linux-muslgnueabi-

	# override TOOLPREFIX only if it is empty
	[ -z "$TOOLPREFIX" ] && TOOLPREFIX=$toolprefix
	export TOOLPREFIX

	echo "$SCRIPT_NAME  KERNEL_DIR=$KERNEL_DIR"
	echo "$SCRIPT_NAME  TOOLPREFIX=$TOOLPREFIX"
	#echo "$SCRIPT_NAME  CHIPSET=$CHIPSET"

	echo "# setting of $(basename $SCRIPT_NAME)  $(date)
TOOLPREFIX=$TOOLPREFIX
CONFIG_TARGET_PRX126=$CONFIG_TARGET_PRX126
OMCI_BM_ENABLE=$OMCI_BM_ENABLE
KERNEL_DIR=$KERNEL_DIR
" > $CURDIR/mk_omcimain_tgz.sh.config
}

platform_x86() {
	echo "platform x86"
	# for x86 platform, the script set TOOLPREFIXTOOLPREFIX=/usr/bin/ for target Makefile

	TOOLPREFIX=/usr/bin/
	export TOOLPREFIX

	echo "$SCRIPT_NAME  TOOLPREFIX=$TOOLPREFIX"

	echo "# setting of $(basename $SCRIPT_NAME)  $(date)
TOOLPREFIX=$TOOLPREFIX
OMCI_BM_ENABLE=$OMCI_BM_ENABLE
" > $CURDIR/mk_omcimain_tgz.sh.config
}

########################################################################

if [ "$TOOLPREFIX" = "/usr/bin/" ]; then
	platform_x86
elif [ "$CONFIG_TARGET_PRX126" = "y" ]; then
	platform_prx126
	export CONFIG_TARGET_PRX126
elif [ "$CONFIG_TARGET_PRX120" = "y" ]; then
	platform_prx120
	export CONFIG_TARGET_PRX120
elif [ "$CONFIG_TARGET_PRX321" = "y" ]; then
	platform_prx321
	export CONFIG_TARGET_PRX321
fi

export OMCI_BM_ENABLE
if [ ! -z "$OMCI_BM_ENABLE" ]; then
	echo "$SCRIPT_NAME  OMCI_BM_ENABLE=$OMCI_BM_ENABLE"
fi

######################################################################

# build omcimain & *.a, *.so
if [ "$CLEAN" = "y" ]; then
	rm $CURDIR/mk_omcimain_tgz.sh.config 2>/dev/null
	# force subdir omci_bm always being cleaned in Non-X86 platform
	[ "$TOOLPREFIX" != "/usr/bin/" ] && OMCI_BM_ENABLE=y
	make clean || exit 1
else
	make || exit 1
fi

##########################################################################

# for omcimain.tgz
cd $CURDIR
if [ "$CLEAN" = "y" ]; then
	[ -f omcimain.tgz ] && rm omcimain.tgz
	exit 0
fi

D=/tmp/omci.$$

mkdir $D
mkdir -p $D/usr/bin
mkdir -p $D/etc/omci
mkdir -p $D/lib

cp $CURDIR/../client_prog/omci_client 	$D/usr/bin/omci
cp $CURDIR/../src/omcimain 		$D/usr/bin/omcimain
cp $CURDIR/../../../../fvt/files/fw_upgrade.sh      $D/usr/bin/fw_upgrade.sh

if [ "$CONFIG_TARGET_PRX126" = "y" ]; then
	cp $CURDIR/../../../../fvt/files/laser_driver_util.sh      $D/usr/bin/laser_driver_util.sh
	cp $CURDIR/../../../../fvt/files/2182.txt      $D/usr/bin/2182.txt
fi

${TOOLPREFIX}strip $D/usr/bin/omcimain $D/usr/bin/omci


# create symbolic link for backward compatibility
cd $D/usr/bin
ln -s omci ./omcicli
ln -s omci ./gpon_monitor
cd $CURDIR

# get proprietary list
proprietary_list=`ls -d ../etc/omci/* | cut -d'/' -f4`

# copy script, xml,xdb files to tmp
(cd $CURDIR/../etc; tar --exclude=.svn -cf - *)|(cd $D/etc; tar -xf -)
for dir in $proprietary_list; do
	# chmod for script
	if [ -d $D/etc/omci/$dir/script ]; then
		chmod 755 $D/etc/omci/$dir/script/*
	fi
	# convert spec xml to xdb
	if [ -f ./convert_xml.sh ]; then
		[ -d $D/etc/omci/$dir/spec ] &&  sh ./convert_xml.sh $D/etc/omci/$dir/spec
	fi
done

# create symbolic link
cd $D/etc/omci/00base/script
ln -s /usr/sbin/sw_download.sh sw_download.sh
cd $CURDIR

cat $CURDIR/omcienv.xml | \
	sed -e "s/<transport_type>1/<transport_type>0/" | \
	sed -e "s/<etc_omci_path>\.\./<etc_omci_path>/" | \
	sed -e "s/<etc_omci_path2>\.\./<etc_omci_path2>/" | \
	sed -e "s/<anig_type>./<anig_type>9/" | \
	sed -e "s/<voip_enable>./<voip_enable>9/" > $D/etc/omci/omcienv.xml
cp $D/etc/omci/omcienv.xml $D/etc/omci/omcienv.xml.sample

cd $D
find . -name ".git*" -exec rm {} \;
tar -zcvf $CURDIR/omcimain.tgz *

cd $CURDIR
rm -Rf $D
echo "$CURDIR/omcimain.tgz is generated for $CHIPSET"
