# Makefile for src/wx/common directory
# $Id$

include ../../../makeopts

SRC	:= phone.cpp vcard.cpp 
TARGETS := $(SRC:.cpp=.o)

all:	$(TARGETS)
	@true

include ../../../makerules $(wildcard *.d)

clean: stdclean

.PHONY: clean all
