# Names.mk for wx/common directory
# $Id$

SRC :=	$(patsubst .src/%,%,$(wildcard .src/wx/common/*.cpp))

MOBJS	+= $(SRC:.cpp=.o)
MSGSRC	+= $(SRC)
