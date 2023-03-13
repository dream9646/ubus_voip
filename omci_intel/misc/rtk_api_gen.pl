#!/bin/perl

# usage
# cat `ls */*.c|grep  -v _rg_` |grep rtk |perl misc/rtk_api_gen.pl |sort |uniq -c |sort -k2 > misc/rtk_api_usage.txt

while (<>) {
	if ($_ =~ m/(rtk_[A-Za-z_0-9]+)\(/) {
		print "$1\n";
	}
}

	