#1/bin/sh
CURDIR=`pwd`
echo convert xml to xdb...
if [ ! -f ../util/fencrypt.x86 ]; then
	gcc -DMAKE_FENCRYPT -O2 -o ../util/fencrypt.x86 ../util/fencrypt.c 
fi

for XMLDIR in $*; do
	if [ ! -d $XMLDIR ]; then
		echo $XMLDIR not found?
		continue
	fi
	
	cd $XMLDIR
	for f in *.xml; do
		x=`echo $f|sed -e "s/xml/xdb/"`
		if $CURDIR/../util/fencrypt.x86 $f $x xmlomci; then
			echo convert $f to $x ok
			rm $f
		else
			echo convert $f to $x err!
		fi
	done
	cd $CURDIR
done
