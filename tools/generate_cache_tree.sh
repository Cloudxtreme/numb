#!/usr/local/bin/bash

for i in 0 1 2 3 4 5
do
    mkdir $i
    cd $i
    j=0
    while [ $j -lt 1024 ]
    do
        d=
        if [ $j -lt 10 ] ; then d=000$j
        elif [ $j -lt 100 ] ; then d=00$j
        elif [ $j -lt 1000 ] ; then d=0$j
        else d=$j
        fi
        mkdir $d
        j=$(($j+1))
    done
    cd -
done