# Makefile for libversit library
# $Id$

include ../../../makeopts

TARGETS = libversit.a

CPPFLAGS += -I.src

all:	$(TARGETS)
	@true


include ../../../makerules $(wildcard *.d)

msgcat:
	@echo nothing to be done

libversit_a_OBJECTS = vcc.o vobject.o # vcaltmp.o

libversit.a: $(libversit_a_OBJECTS)
	$(RM) libversit.a
	$(AR) cru libversit.a $(libversit_a_OBJECTS)
	$(RANLIB) libversit.a

clean: stdclean
	rm -f vcc.c


.PHONY: clean all
