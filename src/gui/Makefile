# Makefile for src/gui directory

CWD = gui
SRC = wxMApp.cpp wxMFrame.cpp wxFolderView.cpp wxMessageView.cpp \
wxComposeView.cpp wxMainFrame.cpp wxIconManager.cpp wxMDialogs.cpp  \
wxlwindow.cpp wxllist.cpp wxlparser.cpp wxMenuDefs.cpp wxOptionsDlg.cpp \
wxFolderTree.cpp wxDialogLayout.cpp wxMFolderDialogs.cpp wxBrowseButton.cpp \
wxHeadersDialogs.cpp wxTemplateDialog.cpp wxModulesDlg.cpp

ifdef USE_WXTAB
SRC += wxtab.cpp
endif

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

include ../../makeopts

#XTADDONS = WIN_dnd.cpp WIN_listctrl.cpp WIN_rawlistctrl.cpp WIN_rawtext.cpp

LINK = ../util/strutil.o ../classes/CommonBase.o $(CCLIENTLIB)

TARGETS = $(SRC:.cpp=.o)

all:	$(TARGETS)

$(LINK):
	(cd ../util; make all)
	(cd ../classes; make all)
	(cd $(CCLIENTDIR); make $(CCCLIENTOSTYPE))


clean: stdclean

.PHONY: clean

include ../../makerules
include .depend
