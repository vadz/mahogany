# Names.mk for wx/generic directory
# $Id$

SRC :=	$(patsubst .src/%,%,$(wildcard .src/modules/crypt/*.cpp))

MOBJS	+= $(SRC:.cpp=.o)
MSGSRC	+= $(SRC)
