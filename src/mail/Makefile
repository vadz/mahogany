# Makefile for src/mail directory

CWD = mail 
SRC = MailFolder.cc MailFolderCC.cc MessageCC.cc SendMessageCC.cc
HDR = $(SRC:.cc=.h)
DEP = $(SRC:.cc=.d)
OBJ = $(SRC:.cc=.o)

include ../../makeopts

LINK = ../util/strutil.o ../classes/CommonBase.o $(CCLIENTLIB)

TARGETS = $(OBJ)

all:	$(TARGETS)

MailFolderCC: MailFolderCC.o MailFolder.o MessageCC.o $(LINK)
	$(CXX) -o $@ $^

$(LINK):
	(cd ../util; make all)
	(cd ../classes; make all)
	(cd $(CCLIENTDIR); make $(CCCLIENTOSTYPE))


include ../../makerules
include .depend
