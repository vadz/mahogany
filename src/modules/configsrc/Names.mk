###############################################################################
# Project:     M - cross platform e-mail GUI client
# File name:   src/modules/configsrc/Names.mk
# Purpose:     makefile options for this directory
# Author:      Vadim Zeitlin
# Created:     26.08.03
# Version:     $Id$
# Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwindows.org>
# Licence:     M licence
###############################################################################

ifeq (0,1)
SRC	:= $(patsubst .src/%,%,$(wildcard .src/modules/configsrc/*.cpp))
MOD	:= $(SRC:.cpp=.so)

MSOS	+= $(MOD)
MSGSRC	+= $(SRC)
endif
