#!/bin/sh

OPTS="-m 6500 -n 10000 -g 10000 --peaks --graininit --voronei --savemem"

./crystal $OPTS -d long14 -s 14
./crystal $OPTS -d long6  -s 6

