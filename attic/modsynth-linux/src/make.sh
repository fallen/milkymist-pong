#!/usr/bin/env sh

gcc -Wall -DUSE_MMAP -DLITTLE_ENDIAN -D_DEBUG=1 main.c pcm.c mod.c file.c
