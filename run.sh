#!/bin/sh

OPTS="-m 2500 -n 2000 -g 10000 -l -s 6 -r 10"

#generate a graininit
./crystal -d ./ -n 2 -g 10000 -q --graininit --voronei

for diff in 0 0.9 0.95 0.996
do
	for angle in 0 1 2 20 200
	do
		mkdir -p data-$diff-$angle
		cp graininit data-$diff-$angle/
		./crystal $OPTS -d data-$diff-$angle -D $diff -a $angle
#		ffmpeg -i data-$diff-$angle/slope.%05d.png -b 20000k slope-$diff-$angle.mpg
#		ffmpeg -i data-$diff-$angle/level.%05d.png -b 20000k level-$diff-$angle.mpg
	done
done


