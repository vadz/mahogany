# Names.mk for classes directory
# $Id$

SRC :=	$(patsubst .src/%,%,$(wildcard .src/classes/*.cpp))
SRC :=	$(filter-out classes/IconList.cpp, $(SRC))

MOBJS	+= $(SRC:.cpp=.o)
MSGSRC	+= $(SRC)
