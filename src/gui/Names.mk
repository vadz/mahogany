# Names.mk for gui directory
# $Id$

SRC :=	$(patsubst .src/%,%,$(wildcard .src/gui/*.cpp))

ifdef EXPERIMENTAL
CPPFLAGS_gui_wxMDialogs_o := -DEXPERIMENTAL='"$(EXPERIMENTAL)"'
endif

MOBJS	+= $(SRC:.cpp=.o)
MSGSRC	+= $(SRC)
