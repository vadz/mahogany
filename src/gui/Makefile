# Makefile for src/gui directory

CWD = gui
SRC = wxMApp.cc wxMFrame.cc wxFolderView.cc wxMessageView.cc \
wxComposeView.cc wxMainFrame.cc wxFText.cc wxFTCanvas.cc \
wxIconManager.cc wxFontManager.cc wxMDialogs.cc wxMLogFrame.cc \
wxAdbEdit.cc wxtab.cc

HDR = $(SRC:.cc=.h)
DEP = $(SRC:.cc=.d)

include ../../makeopts

#XTADDONS = WIN_dnd.cpp WIN_listctrl.cpp WIN_rawlistctrl.cpp WIN_rawtext.cpp

LINK = ../util/strutil.o ../classes/CommonBase.o $(CCLIENTLIB)

TARGETS = $(SRC:.cc=.o)

all:	$(TARGETS)

$(LINK):
	(cd ../util; make all)
	(cd ../classes; make all)
	(cd $(CCLIENTDIR); make $(CCCLIENTOSTYPE))


include ../../makerules
include .depend
