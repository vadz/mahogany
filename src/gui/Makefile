# Makefile for src/gui directory

SRC = wxMApp.cpp wxMFrame.cpp wxFolderView.cpp \
wxComposeView.cpp wxMainFrame.cpp wxIconManager.cpp wxMDialogs.cpp  \
wxlwindow.cpp wxllist.cpp wxlparser.cpp wxMenuDefs.cpp wxOptionsDlg.cpp \
wxFolderTree.cpp wxDialogLayout.cpp wxMFolderDialogs.cpp wxBrowseButton.cpp \
wxHeadersDialogs.cpp wxTemplateDialog.cpp wxModulesDlg.cpp \
wxSubfoldersDialog.cpp wxFiltersDialog.cpp wxWizards.cpp wxFolderMenu.cpp \
Mdnd.cpp MImport.cpp

include ../../makeopts

ifdef EXPERIMENTAL
SRC += wxMessageViewNew.cpp wxMEditCtrl.cpp
else
SRC += wxMessageView.cpp
endif

TARGETS = $(SRC:.cpp=.o)

all:	$(TARGETS)
	@true

include ../../makerules $(wildcard *.d)

clean: stdclean

.PHONY: clean
