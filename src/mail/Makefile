# Makefile for src/mail directory

include ../../makeopts

SRC = MailFolder.cpp Message.cpp MailFolderCC.cpp MessageCC.cpp \
SendMessageCC.cpp ASMailFolder.cpp MailMH.cpp HeaderInfoImpl.cpp

ifdef EXPERIMENTAL
#commenting out for now as it doesn't do anything and gives tons of warnings
#SRC += MMailFolder.cpp
endif

OBJ = $(SRC:.cpp=.o)

all:	$(OBJ)
	@true

include ../../makerules $(wildcard *.d)

### this part may not work any more...
LINK = ../util/strutil.o ../classes/CommonBase.o $(CCLIENTLIB)

MailFolderCC: MailFolderCC.o MailFolder.o MessageCC.o $(LINK)
	$(CXX) -o $@ $^

$(LINK):
	cd ../util; make all
	cd ../classes; make all
	cd $(CCLIENTDIR); make $(CCCLIENTOSTYPE)

clean: stdclean

.PHONY: clean
