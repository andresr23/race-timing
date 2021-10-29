# The Race-Timing Prototype
This repository contains all the source code used for the paper "**The Race-Timing Prototype**", which has been accepted by The 18th Annual International Conference on Privacy, Security and Trust (PST2021).

## Directories
-`libcache`: Object library that contains all the necessary code to configure a cache-side channel in the L1d cache.
-`kernel-amd-spec`: Special kernel module for AMD processors to activate/deactivate the no-speculation semantics for the `lfence` instruction, which can alter the efficiency of The Race-Timing Prototype.
-`kernel-pmc`: Kernel module designed to activate the Performance-Monitoring Counters (PMCs) used by the `libcache` library.
-`race-timing`: Calibration programs to find suitable race interim and out-of-order detachment values.
-`l1d`: Programs to measure the behavior and efficiency of The Race-Timing Prototype when probing L1d sets.
-`aes-demo`: Attack against a modified version of an AES cipher.

## Building the Project
Building each experiment is as easy as running `make` on each experiment sub-directory. Note however, that the default target of `make` compiles each experiment binary for Intel's i7-6700 (Skylake) by default. To compile for AMD's Ryzen 5 1600x (Zen) simply do `make zen`, or `make zen-spec` in case that `lfence` allows for speculation.

However, some of the functions in `libcache` use PMCs, and therefore must be activated before running any experiment that invokes PMC functions. To do so, navigate to `kernel-pmc`, run either `make` to build a kernel module that activates PMCs in Intel processors, or `make amd` for AMD processors. After building any of the kernel modules, the corresponding directory also includes installation scripts that run by simply doing `./insall.sh`.

Note that installing kernel modules requires super-user privileges.

## Usage
Every experiment contains a dedicated `start.sh` script to run each test after it has been compiled. After running the experiment, the script also invokes `gnuplot` to generate figures similar to those in the paper.

## Disclaimer
The Race-Timing Prototype is a novel framework, and the purpose of this project is to demonstrate that it is sensible enough to detect L1d cache hits/misses that have a very minuscule latency discrepancy. Because of this, race-timing functions are quite sensitive, and they might require re-calibration even after firmware updates.
