# Names.mk for wx/generic directory
# $Id$

SRC	:= $(patsubst .src/%,%,$(wildcard .src/modules/viewflt/*.cpp))
MOD	:= $(SRC:.cpp=.so)

MSOS	+= $(MOD)
MSGSRC	+= $(SRC)
