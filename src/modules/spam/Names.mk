###############################################################################
# Project:     M
# File name:   src/modules/spam/Names.mk
# Purpose:     files to build in src/modules/spam directory
# Author:      Vadim Zeitlin
# Modified by:
# Created:     2004-07-08
# CVS-ID:      $Id$
# Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
# Licence:     M license
###############################################################################

SRC	:= $(patsubst .src/%,%,$(wildcard .src/modules/spam/*.cpp))
MOD	:= $(SRC:.cpp=.so)

MSOS	+= $(MOD)
MSGSRC	+= $(SRC)

