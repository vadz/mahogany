# $Id$
#
# Makefile for libversit library

TARGETS = libversit.a

all: $(TARGETS)

libversit_a_OBJECTS = vcc.o vobject.o # vcaltmp.o

libversit.a: $(libversit_a_OBJECTS)
	-rm -f libversit.a
	$(AR) cru libversit.a $(libversit_a_OBJECTS)
	$(RANLIB) libversit.a

clean: stdclean

.PHONY: clean all

include ../../../makeopts
include ../../../makerules
include .depend

