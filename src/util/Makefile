# Makefile for src/mail directory

CWD = util
SRC = strutil.cc locks.cc
HDR = $(SRC:.cc=.h)
DEP = $(SRC:.cc=.d)

TARGETS = $(SRC:.cc=.o)

all:	$(TARGETS)
strutil.o: strutil.cc

include ../../makeopts
include ../../makerules
