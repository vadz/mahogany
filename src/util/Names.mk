# Names.mk for util directory
# $Id$

SRC	:= $(patsubst .src/%,%,$(wildcard .src/util/*.cpp))
CSRC	:= $(patsubst .src/%,%,$(wildcard .src/util/*.c))
SRC	:= $(filter-out util/LeakTracer.cpp, $(SRC))

MOBJS	+= $(SRC:.cpp=.o) $(CSRC:.c=.o)
MSGSRC	+= $(SRC)

# strutil.cpp #includes some charset files from c-client src/charset directory
CPPFLAGS_util_strutil_o := -I$(SOURCEDIR)/lib/imap/src
