# Makefile for src/util directory
# $Id$

include ../../makeopts

# Need SRC for translation processing
SRC	:= $(patsubst .src/%,%,$(wildcard .src/*.cpp))
TARGETS	:= strutil.o sysutil.o upgrade.o miscutil.o twofish2.o ssl.o

ifdef USE_LEAKTRACER
TARGETS	+= LeakTracer.o
endif

all:	$(TARGETS)
	@true

include ../../makerules $(wildcard *.d)

clean: stdclean

.PHONY: all clean
