# Makefile for src/mail directory

CWD = util
SRC = strutil.cc #locks.cc
CSRC = #pyembed0.c pyembed1.c pyembed2.c pyembed3.c pyembed4.c
HDR = $(SRC:.cc=.h) $(CSRC:.c=.h)

TARGETS = $(SRC:.cc=.o) $(CSRC:.c=.o)

all:	$(TARGETS)
strutil.o: strutil.cc

include ../../makeopts
include ../../makerules
