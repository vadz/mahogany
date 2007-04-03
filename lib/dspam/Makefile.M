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

include ../../makeopts

LIBDSPAM=src/.libs/libdspam.a
DRIVER=src/.libs/libhash_drv.a
MAKEFILE_DSPAM=Makefile

all:	$(LIBDSPAM) $(DRIVER)

# explanation of configure arguments:
#   --with-delivery-agent is needed because dspam can't always detect delivery
#       agent automatically, as we don't use it anyhow just choose a dummy one
#
#   --disable-shared is needed because we want to link everything statically
#       into our DspamFilter.so module and SHARED_CFLAGS is needed because it
#       contains -fPIC when building the modules dynamically and we have to
#       build all code linked into a DSO with -fPIC
$(MAKEFILE_DSPAM): .src/$(MAKEFILE_DSPAM).in
	.src/configure CFLAGS=$(SHARED_CFLAGS) --disable-shared \
	               --with-delivery-agent=/bin/false -C

$(LIBDSPAM) $(DRIVER): $(MAKEFILE_DSPAM)

$(LIBDSPAM):
	$(MAKE) -C src libdspam.la

$(DRIVER):
	$(MAKE) -C src libhash_drv.la

clean:
	$(MAKE) -C src clean

.PHONY: all clean
