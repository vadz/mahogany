# Makefile for src/mail directory

CWD = util
SRC = strutil.cpp #locks.cpp
CSRC = #pyembed0.c pyembed1.c pyembed2.c pyembed3.c pyembed4.c
HDR = $(SRC:.cpp=.h) $(CSRC:.c=.h)

TARGETS = $(SRC:.cpp=.o) $(CSRC:.c=.o)

all:	$(TARGETS)
strutil.o: strutil.cpp

include ../../makeopts
include ../../makerules
include .depend
