# Makefile for src/gui directory

CWD = gui
SRC = wxMApp.cpp wxMFrame.cpp wxFolderView.cpp \
wxComposeView.cpp wxMainFrame.cpp wxIconManager.cpp wxMDialogs.cpp  \
wxlwindow.cpp wxllist.cpp wxlparser.cpp wxMenuDefs.cpp wxOptionsDlg.cpp \
wxFolderTree.cpp wxDialogLayout.cpp wxMFolderDialogs.cpp wxBrowseButton.cpp \
wxHeadersDialogs.cpp wxTemplateDialog.cpp wxModulesDlg.cpp \
wxSubfoldersDialog.cpp wxFiltersDialog.cpp wxWizards.cpp wxFolderMenu.cpp \
Mdnd.cpp MImport.cpp 

include ../../makeopts

ifdef EXPERIMENTAL
SRC += wxMessageViewNew.cpp
else
SRC += wxMessageView.cpp
endif

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

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
