# Names.mk for mail directory
# $Id$

SRC :=	$(patsubst .src/%,%,$(wildcard .src/mail/*.cpp))
SRC :=	$(filter-out mail/SendMessage.cpp, $(SRC))
SRC :=	$(filter-out mail/MMailFolder.cpp, $(SRC))

ifdef EXPERIMENTAL
#commenting out for now as it doesn't do anything and gives tons of warnings
#SRC +=	mail/MMailFolder.cpp
endif

MOBJS	+= $(SRC:.cpp=.o)
MSGSRC	+= $(SRC)
