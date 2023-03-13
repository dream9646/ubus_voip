#!/bin/sh

# directory we care
#hw_dir_list="acl_hw_fvt2510 cpuport_hw_fvt2510 er_group_hw gpon_hw_fvt2510 hwresource meinfo_hw switch_hw_fvt2510 voip_hw"
hw_dir_list="acl_hw_fvt2510 gpon_hw_fvt2510 switch_hw_fvt2510 cpuport_hw_fvt2510"
pid=$$
# get file list
for dir in $hw_dir_list; do
	for filename in $dir/*.[ch] ; do
		echo $filename
		./misc/rg_conv.pl $filename > ./rg_conv.$pid
		mv ./rg_conv.$pid $filename
	
		#revert if translation fail
		###git checkout $filename
	done
done
