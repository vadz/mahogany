# Makefile for locale/wxstd directory
# $Id$

FILES_MO:=$(patsubst .src/%.po,%.mo,$(wildcard .src/*.po))

include ../../makeopts

vpath %.po .src

%.mo: %.po
	$(MSGFMT) -o $@ $<

all:	$(FILES_MO)
	@true

clean:
	$(RM) $(FILES_MO)

.PHONY: all dep depend clean
