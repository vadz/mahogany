###############################################################################
# Project:     M
# File name:   lib/Makefile
# Purpose:     Makefile for 3rd party libraries used by Mahogany
# Author:      Vadim Zeitlin
# Modified by:
# Created:     2004-05-01
# CVS-ID:      $Id$
# Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
# Licence:     M license
###############################################################################

include ../makeopts

SUB_DIRS := compface dspam imap

all clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i -f Makefile.M $@; done

install:
	@true

.PHONY: all clean
