#!/bin/bash
PLATFORM=${1:-"skylake"}

TARGET="pstates"

BINARY="${TARGET}.out"
GNPLOT="${TARGET}.gp"

FINALF="${TARGET}-${PLATFORM}.dat"

taskset -c 0 ./$BINARY > $FINALF
wait

gnuplot -e "platform='$PLATFORM'" $GNPLOT
