# Makefile for src/wx/xxx directory
# $Id$

include ../../../makeopts

SRC	:= persctrl.cpp vcarddlg.cpp
TARGETS	:= $(SRC:.cpp=.o)

all:	$(TARGETS)
	@true

include ../../../makerules $(wildcard *.d)

clean: stdclean

.PHONY: clean all
