# Makefile for src/util directory
# $Id$

CWD = util
SRC = strutil.cpp sysutil.cpp upgrade.cpp miscutil.cpp
CSRC =
HDR = $(SRC:.cpp=.h) $(CSRC:.c=.h)

TARGETS = $(SRC:.cpp=.o) $(CSRC:.c=.o)

include ../../makeopts

ifdef USE_LEAKTRACER
TARGETS := $(TARGETS) LeakTracer.o
endif

all:	$(TARGETS)

clean: stdclean

.PHONY: all clean

include ../../makerules
include .depend
