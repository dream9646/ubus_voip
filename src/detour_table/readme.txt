
Usage: detour_table [-d debug_level] [-f|o|p filename]
List directory contents
         -d     debug_level 0:disable 1: error only 2:debug; default:1
         -f     input detour table file name
         -o     output detour table file name
         -p     output dialing plan file name



Example for valid input file:
jku@ipo12 detour_table$ ./detour_table -f test_detour.005.01.1.DT001 
jku@ipo12 detour_table$ ls -l test_detour.*
-rw-r--r-- 1 jku fvrd 2128 Jul 25 14:46 test_detour.005.01.1.DT001
-rw-r--r-- 1 jku fvrd 1939 Jul 27 14:05 test_detour.005.01.1.DT001.valid

./detour_table -f test_detour.005.01.1.DT001 -p test_detour.005.01.1.DT001.plan
jku@ipo12 detour_table$ echo $?
0
jku@ipo12 detour_table$ ls -l test_detour.*
-rw-r--r-- 1 jku fvrd 2128 Jul 25 14:46 test_detour.005.01.1.DT001
-rw-r--r-- 1 jku fvrd 3402 Jul 27 14:09 test_detour.005.01.1.DT001.plan
-rw-r--r-- 1 jku fvrd 1939 Jul 27 14:09 test_detour.005.01.1.DT001.valid

check:
jku@ipo12 detour_table$ diff -uBb test_detour.005.01.1.DT001 test_detour.005.01.1.DT001.valid 


Example for error input file:
jku@ipo12 detour_table[master*]$ ./detour_table -d1 -f test_err.005.01.1.DT001
Error on item [007]
jku@ipo12 detour_table[master*]$ echo $?
10

check header file we found: #define STATE_FORMAT_ERR        10
another err test example:
jku@ipo12 detour_table[master*]$ ./detour_table -d2 -f test_err2.006.01.1.DT001
001: 006.01.1.DT001
002: 8, 8
[003] [P]       [#]     [5]
#xxxx@pstn_dial
[004] [P]       [*]     [0]
*x.@pstn_dial
[005] [P]       [0032]  [0]
0032x.@pstn_dial
Error digit len [006] [S]       [8888]  [3]
jku@ipo12 detour_table[master*]$ echo $?
15

Normal step:
VoIP need file: test_detour.005.01.1.DT001.plan
ex:

/opt/voip/bin/detour_table -f test_detour.005.01.1.DT001 -p /var/run/voip/test_detour.005.01.1.DT001.plan
fvcli -p set specialdigitmapdelall 0
fvcli -p set specialdigitmapaddfile 0 /var/run/voip/test_detour.005.01.1.DT001.plan


