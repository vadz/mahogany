###############################################################################
# Project:     M
# File name:   lib/imap/Makefile.M
# Purpose:     trampoline makefile for c-client
# Author:      Vadim Zeitlin (based on old extra/src/c-client/Makefile)
# Modified by:
# Created:     2004-05-01
# CVS-ID:      $Id: Makefile.M,v 1.2 2004/07/08 21:57:17 vadz Exp $
# Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
# Licence:     M license
###############################################################################

# This is the "real" Makefile for the c-client directory.  It kludges
# around the Makefile provided with the library, coercing it to work
# with the Mahogany build system.

include ../../makeopts

# we depend upon the fact that the last file created by c-client's
# setup is $(OSDEP), so if it is present, we're ready to go.
OSDEP := c-client/osdep.c

# linkage.c file is normally included by user code to initialize c-client, we
# use a slightly modified version of it
LINKAGE=c-client/linkage_no_ssl.c

# c-client cannot use a parallel make, so strip off any -j arguments to
# environmental MAKE
MAKE := $(shell echo $(MAKE) | sed "1,1s/-j *[0-9]*//g")

LIBRARY := c-client/c-client.a

all:	$(LIBRARY) $(LINKAGE)
	@true

# initial make call, prepare the scene for the invocation of c-client/Makefile
# 
# 1. create the links to the files in the source directory
# 2. create helper files (SPECIALS, ...)
# 3. fix for the problem with files not having executable bit in the cvs
#
# NB: can't use "ls -sf" in step 1 because it doesn't overwrite existing files
#     under Solaris (don't ask me why), hence test for files existence
#     explicitly
IMAP_SRC_DIR := .src/src
IMAP_UNIX_DIR := $(IMAP_SRC_DIR)/osdep/unix
$(OSDEP):
	@if [ ! -d c-client ]; then mkdir c-client; fi
	for i in $(IMAP_UNIX_DIR)/Makefile \
		$(IMAP_UNIX_DIR)/drivers \
		$(IMAP_UNIX_DIR)/mkauths \
		$(IMAP_SRC_DIR)/c-client/*.[ch] \
		$(IMAP_SRC_DIR)/ansilib/*.c \
		$(IMAP_SRC_DIR)/charset/*.c \
		$(IMAP_UNIX_DIR)/*.[ch] \
		; do \
	    if test ! -f c-client/`basename $$i`; then ln -s ../$$i c-client; fi \
	done
	@touch c-client/SPECIALS
	@chmod +x c-client/drivers c-client/mkauths
	@$(MAKE) -C c-client $(IMAP_OSTYPE) --no-print-directory \
		MAKE="$(MAKE)" CC="$(CC)" \
		$(IMAP_SSLTYPE) \
		EXTRACFLAGS="$(IMAP_CPPFLAGS)"

# subsequent make call, only call c-client's Makefile if it looks
# like there's something to do
$(LIBRARY): $(OSDEP) $(wildcard c-client/*.c) $(wildcard c-client/*.h)
	@$(MAKE) -C c-client --no-print-directory MAKE="$(MAKE)"

# we don't want to call ssl_onceonlyinit() from linkage.c, instead we do it
# ourselves later (the first time SSL is needed)
#
# c-client also disables plain passwords if using SSL by default, prevent this
# from happening
$(LINKAGE): c-client/linkage.c
	grep -v ssl_onceonlyinit $< | grep -v SET_DISABLEPLAINTEXT > $@

c-client/linkage.c: $(OSDEP)
	@true

clean:
	@if test -d c-client; then \
		$(MAKE) -C c-client --no-print-directory clean; \
	fi
	$(RM) -r c-client

.PRECIOUS: $(OSDEP)
.PHONY: all clean veryclean
