#!/bin/sh
key=$1
value=$2
ep=$3
#rtp_port_base and rtp_port_limit would use cmd_list.sh to combinate these two value
if [ "$key" = "rtpportbase" ]; then
  rtp_port_limit=$(uci get evoip.GENERAL.rtp_port_limit)
  echo "fvcli set rtpportlimits $value $rtp_port_limit"
elif [ "$key" = "rtpportlimit" ]; then
  rtp_port_base=$(uci get evoip.GENERAL.rtp_port_base)
  echo "fvcli set rtpportlimits $rtp_port_base $value"
else
  echo "key=$key, value=$value, ep=$ep"
fi
