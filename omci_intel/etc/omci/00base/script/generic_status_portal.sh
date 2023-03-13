#!/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/etc/init.d:/root:/root/tool; export PATH
XTABLES_LIBDIR=/lib/xtables; export XTABLES_LIBDIR

interface=pon0

# env vars passed from caller (er_group_hw/er_group_hw_330.c)
#
# ve_interdomain_name
# ve_iana_assigned_port
# tcpudp_protocol
# tcpudp_port
# tcpudp_tosdscp
# iphost_ipaddr
# xmlfile
#
# this script is expected to utilize the above environment variables to 
# connect to a remote manage server and get the configuration and status into xml files.
# the output should be put to the filename specified by $xmlfile in xml format
#

me_init () {
}

me_release () {
}

get_config () {
}

get_status () {
}


#########################

usage () {
	echo "$0 add|del|get_status|get_config"
}

# multiple action could be specified in one line...
for action in $*; do 
	case $action in
		add)		me_init;;
		del)		me_release;;
		get_cofig)	get_config;;
		get_status)	get_status;;
		*)		usage; exit 1;;
	esac
done
