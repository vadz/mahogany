# Makefile for src/classes directory
# $Id$

include ../../makeopts

SRC :=	MObject.cpp MApplication.cpp PathFinder.cpp Profile.cpp \
	XFace.cpp kbList.cpp MFolder.cpp MEvent.cpp MailCollector.cpp \
	MModule.cpp MessageTemplate.cpp MFilter.cpp

TARGETS = $(SRC:.cpp=.o)

all:	$(TARGETS)
	@true

include ../../makerules $(wildcard *.d)

clean: stdclean

.PHONY: clean
