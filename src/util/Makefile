# Makefile for src/util directory
# $Id$

include ../../makeopts

TARGETS = strutil.o sysutil.o upgrade.o miscutil.o

ifdef USE_LEAKTRACER
TARGETS := $(TARGETS) LeakTracer.o
endif

all:	$(TARGETS)
	@true

include ../../makerules $(wildcard *.d)

clean: stdclean

.PHONY: all clean
