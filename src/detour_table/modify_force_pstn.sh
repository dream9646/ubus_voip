#!/bin/sh

#force_pstn_string=$1
#detour_table_input_file=$2
#detour_table_output_file=$3

default_force_pstn="0000,4"

#####test####
detour_table_input_file=test_force_pstn.005.01.1.DT001
detour_table_output_file=test.txt.tmp

#example:
#force_pstn_string="22222,5"
force_pstn_string="5432,4"
#force_pstn_string="0000,4"
#### end test ######

tmp_file=$detour_table_output_file.$$

if grep -q "^S,.*" $detour_table_input_file ; then
	echo "force pstn already exist, replace string"
	sed -e "s/^S,.*/S,$force_pstn_string/" $detour_table_input_file > $detour_table_output_file
else
	#no force pstn exist
	if [ "$force_pstn_string" == "$default_force_pstn" ]; then
		echo "fit default value, do nothing"
		echo ""
	else
		echo "add new line and increase line number"
		cp $detour_table_input_file $tmp_file
		sed -i "3i\\S,$force_pstn_string" $tmp_file

		line_num=`grep "^#" $tmp_file | sed -e "s/^#//" | sed -e "s/\r//"`
		line_num=`expr $line_num + 1`
		sed -e "s/^#.*/#$line_num/" $tmp_file > $detour_table_output_file
		rm -f $tmp_file
	fi

fi

