# Makefile for src/classes directory

CWD = Python
SRC = MApplication.i Profile.i
HDR = $(SRC:.cc=.h)
DEP = $(SRC:.cc=.d)

TARGETS = $(SRC:.i=.o)

all:	$(TARGETS)




.PHONY: mkswig all

include ../../makeopts
include ../../makerules

