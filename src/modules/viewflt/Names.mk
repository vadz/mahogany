# Names.mk for wx/generic directory
# $Id$

SRC :=	$(patsubst .src/%,%,$(wildcard .src/modules/viewflt/*.cpp))

MOBJS	+= $(SRC:.cpp=.o)
MSGSRC	+= $(SRC)
