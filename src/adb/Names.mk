# Names.mk for adb directory
# $Id$

# collect the names
SRC :=	$(patsubst .src/%,%,$(wildcard .src/adb/*.cpp))
MOD :=	$(wildcard .src/adb/Import*.cpp) $(wildcard .src/adb/Export*.cpp)
MOD :=	$(patsubst .src/%,%,$(MOD))

# correct the names
SRC :=	$(filter-out $(MOD), $(SRC))
MOD :=	$(filter-out adb/ImportEudora.cpp, $(MOD))

# FIXME: should be a specific value rather than wildcard
ifeq "$(filter %, $(DEBUG))" ""
# don't compile dummy module unless debugging
SRC :=	$(filter-out adb/ProvDummy.cpp, $(SRC))
endif

MSGSRC	+= $(SRC) $(MOD)
MOBJS	+= $(SRC:.cpp=.o)
MSOS	+= $(MOD:.cpp=.so)
