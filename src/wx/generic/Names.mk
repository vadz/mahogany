# Names.mk for wx/generic directory
# $Id$

SRC :=	$(patsubst .src/%,%,$(wildcard .src/wx/generic/*.cpp))

MOBJS	+= $(SRC:.cpp=.o)
MSGSRC	+= $(SRC)
