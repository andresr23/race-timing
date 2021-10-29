#!/bin/bash
PLATFORM=${1:-"skylake"}

TARGET="explore-mc"

BINARY="${TARGET}.out"
GNPLOT="${TARGET}.gp"

OUTPUT0="${TARGET}-acc.dat"
OUTPUT1="${TARGET}-pos.dat"
FINALF0="${TARGET}-acc-${PLATFORM}.dat"
FINALF1="${TARGET}-pos-${PLATFORM}.dat"

FINALF2="${TARGET}-report-${PLATFORM}.txt"

taskset -c 0 ./$BINARY > $FINALF2
wait

mv -f $OUTPUT0 $FINALF0
mv -f $OUTPUT1 $FINALF1

gnuplot -e "platform='${PLATFORM}'" $GNPLOT
