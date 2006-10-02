###############################################################################
# Project:     M
# File name:   lib/compface/Makefile.M
# Purpose:     makefile for compface library
# Author:      Vadim Zeitlin
# Modified by:
# Created:     2004-05-02
# CVS-ID:      $Id: Makefile.M,v 1.2 2004/07/09 10:22:58 vadz Exp $
# Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
# Licence:     M license
###############################################################################

include ../../makeopts

CPPFLAGS=$(CFACE_CPPFLAGS)
CFLAGS=$(CFACE_CFLAGS)

OBJECTS=arith.o file.o compress.o gen.o uncompface.o compface.o

LIBRARY=libcompface.a

all:		$(LIBRARY)
	@true

include ../../makerules $(wildcard *.d)

$(LIBRARY):	$(OBJECTS)
	ar rc $(LIBRARY) $(OBJECTS)
	-ranlib $(LIBRARY)

arith.o:	arith.c compface.h data.h
cmain.o:	cmain.c compface.h data.h
compface.o:	compface.c compface.h data.h
compress.o:	compress.c compface.h data.h
file.o:		file.c compface.h data.h
gen.o:		gen.c compface.h data.h
uncompface.o:	uncompface.c compface.h data.h

clean:
	rm -f $(OBJECTS) $(LIBRARY) $(OBJECTS:.o=.d)

.PHONY: all clean
