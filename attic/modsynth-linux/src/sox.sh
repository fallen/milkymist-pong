#!/usr/bin/env sh

# input file must have the .s16 extension

sox -r 48000 -t raw -s -2 -c 2 $1 ${1/\.s16/.wav}
