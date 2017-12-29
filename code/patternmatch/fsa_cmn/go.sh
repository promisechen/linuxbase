#!/bin/sh

i=1
while [ $i -le 100 ]
do
    ./fsatest 1000 sega.txt
    i=`expr $i + 1`
done
