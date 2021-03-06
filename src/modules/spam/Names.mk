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

ifdef USE_DSPAM
DSPAM_CPPFLAGS=-DHAVE_CONFIG_H -DLOGDIR="" -DCONFIG_DEFAULT=NULL \
		-I../lib/dspam/src -I../lib/dspam/.src/src
DSPAM_LIBS=../lib/dspam/src/.libs/libdspam.a ../lib/dspam/src/.libs/libhash_drv.a

ifeq ($(USE_MODULES),static)
CPPFLAGS_modules_spam_DspamFilter_o := $(DSPAM_CPPFLAGS)
LDFLAGS := $(LDFLAGS) $(DSPAM_LIBS)
else
CPPFLAGS_modules_spam_DspamFilter_so := $(DSPAM_CPPFLAGS)
LDFLAGS_modules_spam_DspamFilter_so := $(DSPAM_LIBS)
endif
else
SRC	:= $(filter-out modules/spam/DspamFilter.cpp, $(SRC))
endif

MOD	:= $(SRC:.cpp=.so)

MSOS	+= $(MOD)
MSGSRC	+= $(SRC)
