# Makefile for src/util directory
# $Id$

CWD = util
SRC = strutil.cpp upgrade.cpp
CSRC =
HDR = $(SRC:.cpp=.h) $(CSRC:.c=.h)

TARGETS = $(SRC:.cpp=.o) $(CSRC:.c=.o)

all:	$(TARGETS)

clean: stdclean

.PHONY: all clean

include ../../makeopts
include ../../makerules
include .depend
