# Names.mk for modules directory
# $Id$
#
# To create a new module for Mahogany, simply start from Mdummy.cpp
# and modify it. Save your Module's source in this directory and run
# make. The Makefile will find it automatically, there is no need to
# edit this file.
#######################################################################

include $(wildcard .src/modules/*/Names.mk) $(wildcard $(patsubst .src/%/Names.mk,%/*.d,$(wildcard .src/modules/*/Names.mk)))

SRC	:= $(patsubst .src/%,%,$(wildcard .src/modules/*.cpp))

# unfortunately Calendar.cpp doesn't work any more (if it ever has, I'm not
# even sure about it) and I prefer to disable even building it until I have
# time to look at it in details
SRC	:=	$(filter-out modules/Calendar.cpp, $(SRC))

MOD	:= $(SRC:.cpp=.so)

ifdef PISOCK_LIB
LDFLAGS_modules_PalmOS_so := $(PISOCK_LIB)
ifdef LIBMAL_SRC
CPPFLAGS_modules_PalmOS_o := -DMALSYNC -DHAVE_LIBMAL -I$(LIBMAL_SRC)
CPPFLAGS_modules_PalmOS_so := -DMALSYNC -DHAVE_LIBMAL -I$(LIBMAL_SRC)
endif
else
MOD	:= $(filter-out modules/PalmOS.so, $(MOD))
endif

ifdef USE_DSPAM
CPPFLAGS_modules_spam_dspam_so := -I../lib/dspam/.src
LDFLAGS_modules_spam_dspam_so := ../lib/dspam/libdspam.a -lsqlite
else
MOD	:= $(filter-out modules/spam/dspam.so, $(MOD))
endif

MSOS	+= $(MOD)

MSGSRC	+= $(SRC)

CLEAN	+= modules/*.exe
