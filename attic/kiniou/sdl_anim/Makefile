#general project structure
SRC:=src
OUT:=out
DATA:=data
OBJS:=$(OUT)
MMDIR?=../../milkymist/milkymist-core


#whether or not to generate the run link (yes/no)
GEN_RUN:=yes

#flags for different builds 
BASE_FLAGS:=-Wall
BASE_LFLAGS:=-lSDL
DEBUG_FLAGS:=-g
DEBUG_LFLAGS:=
FINAL_FLAGS:=-O3
FINAL_LFLAGS:=

#the executable to build to
TARGET:=$(OUT)/sdl_anim

include ./makefile-core
