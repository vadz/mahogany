# Makefile for src/mail directory

CWD = mail 
SRC = MailFolder.cpp Message.cpp MailFolderCC.cpp MessageCC.cpp SendMessageCC.cpp
HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)
OBJ = $(SRC:.cpp=.o)

include ../../makeopts

LINK = ../util/strutil.o ../classes/CommonBase.o $(CCLIENTLIB)

TARGETS = $(OBJ)

ifdef EXPERIMENTAL
TARGETS += ASMailFolder.o
endif

all:	$(TARGETS)

MailFolderCC: MailFolderCC.o MailFolder.o MessageCC.o $(LINK)
	$(CXX) -o $@ $^

$(LINK):
	(cd ../util; make all)
	(cd ../classes; make all)
	(cd $(CCLIENTDIR); make $(CCCLIENTOSTYPE))


clean: stdclean

.PHONY: clean

include ../../makerules
include .depend
