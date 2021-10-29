#!/bin/bash
[ "$UID" -eq 0 ] || exec sudo bash "$0" "$@"

cd module && insmod amd-spec-module.ko
