# Makefile for libversit library
# $Id$

include ../../../makeopts

TARGETS := vcc.o vobject.o # vcaltmp.o

CPPFLAGS += -I.src

all:	$(TARGETS)
	@true

include ../../../makerules $(wildcard *.d)

clean: stdclean
	rm -f vcc.c

.PHONY: clean all
