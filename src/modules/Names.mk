# Names.mk for modules directory
# $Id$
#
# To create a new module for Mahogany, simply start from Mdummy.cpp
# and modify it. Save your Module's source in this directory and run
# make. The Makefile will find it automatically, there is no need to
# edit this file.
# (KB)
#
# Don't forget to add a .mmd (Mahogany Module Definition) file, which
# must contain the following lines:
# Mahogany-Module-Definition
# Name: Modulename
# Version: any kind of version id
# Author: who wrote it
# <empty line>
# Any descriptive text...
#
#######################################################################

SRC	:= $(patsubst .src/%,%,$(wildcard .src/modules/*.cpp))
OBJ	:= $(SRC:.cpp=.so)

ifdef PISOCK_LIB
LDFLAGS_modules_PalmOS_so := $(PISOCK_LIB)
ifdef LIBMAL_SRC
CPPFLAGS_modules_PalmOS_o := -DMALSYNC -DHAVE_LIBMAL -I$(LIBMAL_SRC)
CPPFLAGS_modules_PalmOS_so := -DMALSYNC -DHAVE_LIBMAL -I$(LIBMAL_SRC)
endif
else
OBJ	:= $(filter-out modules/PalmOS.o, $(OBJ))
endif

MSOS	+= $(OBJ)

MSGSRC	+= $(SRC)

CLEAN	+= modules/*.exe
