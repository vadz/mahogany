# Names.mk for util directory
# $Id$

SRC	:= $(patsubst .src/%,%,$(wildcard .src/util/*.cpp))
CSRC	:= $(patsubst .src/%,%,$(wildcard .src/util/*.c))
SRC	:= $(filter-out util/LeakTracer.cpp, $(SRC))

MOBJS	+= $(SRC:.cpp=.o) $(CSRC:.c=.o)
MSGSRC	+= $(SRC)
