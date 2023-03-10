#!/bin/sh

#detour_table_input_file=$1
default_force_pstn="0000"

#####test####
#detour_table_input_file=test_force_pstn.005.01.1.DT001
detour_table_input_file=test.txt.tmp
#### end test ######

if grep -q "^S,.*" $detour_table_input_file ; then
        #echo "force pstn exist"
        grep "^S,.*" $detour_table_input_file | cut -d',' -f 2
else
        #no force pstn exist
        echo "$default_force_pstn"
fi
