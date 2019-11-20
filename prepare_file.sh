#!/bin/sh

file1=$1
file2=$2

timei=$(head -1 $file1 | awk '{print $2}')
timef=$(tail -1 $file1 | awk '{print $2}')
echo $timei

rm kk
cat $file2 | awk -v var="$timei" '{if($3>var) print $0;}' | awk -v var="$timef" '{if($3<(var+100000)) print $0;}' >> kk 
mv kk $file2
