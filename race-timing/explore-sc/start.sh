#!/bin/bash
PLATFORM=${1:-"skylake"}

EXPERIMENT="explore-sc"

BINARY="${EXPERIMENT}.out"
GNPLOT="${EXPERIMENT}.gp"

OUTPUT0="${EXPERIMENT}-acc.dat"
OUTPUT1="${EXPERIMENT}-pos.dat"
FINALF0="${EXPERIMENT}-acc-${PLATFORM}.dat"
FINALF1="${EXPERIMENT}-pos-${PLATFORM}.dat"
FINALF2="${EXPERIMENT}-report-${PLATFORM}.txt"

taskset -c 0 ./$BINARY > $FINALF2
wait

mv -f $OUTPUT0 $FINALF0
mv -f $OUTPUT1 $FINALF1

gnuplot -e "platform='${PLATFORM}'" $GNPLOT
