###############################################################################
# Project:     M
# File name:   lib/dspam/Makefile.M
# Purpose:     makefile for dspam library built as part of M
# Author:      Vadim Zeitlin
# Created:     2004-07-08
# CVS-ID:      $Id: Makefile.M,v 1.8 2006/05/03 17:18:01 vadz Exp $
# Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
# Licence:     M license
###############################################################################

LIBDSPAM=src/.libs/libdspam.a
DRIVER=src/.libs/libhash_drv.a
MAKEFILE_DSPAM=Makefile

all:	$(LIBDSPAM) $(DRIVER)

$(MAKEFILE_DSPAM): .src/$(MAKEFILE_DSPAM).in
	@# dspam can't always detect delivery agent automatically, as we don't
	@# use it anyhow just choose a dummy one
	.src/configure --disable-shared --with-delivery-agent=/bin/false -C

$(LIBDSPAM) $(DRIVER): $(MAKEFILE_DSPAM)

$(LIBDSPAM):
	$(MAKE) -C src libdspam.la

$(DRIVER):
	$(MAKE) -C src libhash_drv.la

clean:
	$(MAKE) -C src clean

.PHONY: all clean
