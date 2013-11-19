#!/bin/bash
if [ "$1" = "" ]; then
 echo usage: $0 Verbrauch/W
 exit
fi

case "$1" in

0) 	echo wrong index
	;;
1)	echo add Meter $1
	vzclient -u bbe93480-0d8b-11e3-b4e1-536f76fe5f15 add data value=$2 > /dev/null
	;;
2)	echo add Meter $1
	#echo Verbrauch Stromzaehler mit Aufloesung 1000 Preis 0.00028
	vzclient -u ab8cba30-0d8b-11e3-81a5-9531f4f32f03 add data value=$2 > /dev/null
	;;
*)	echo add Meter $1
	;;
esac
