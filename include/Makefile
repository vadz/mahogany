# Makefile for include directory

MID:=$(wildcard .src/*.mid)
M4DEF:=$(wildcard .src/*.m4)
IDLFILES:=$(patsubst .src/%.mid,%.idl,$(MID))
INTERFACES:=$(IDLFILES:.idl=.h)
CPPFILES:=$(IDLFILES:.idl=.cpp)

include ../makeopts

vpath %.mid .src

%.h: %.mid
	$(M4) -DM4FILE=.src/mid2h.m4 >$@ $<

%.cpp: %.mid
	$(M4) -DM4FILE=.src/mid2cpp.m4 >$@ $<

%.idl: %.mid
	$(M4) -DM4FILE=.src/mid2idl.m4 >$@ $<

all:	interface
	@true

interface: $(IDLFILES) $(INTERFACES) $(CPPFILES)

install:
	@true

$(IDLFILES) $(INTERFACES) $(CPPFILES): $(MID) $(M4DEF)

msgcat:
	@touch $(BUILDDIR)/imessages.po
	@set -e; for i in *.h gui/*.h adb/*.h ; \
	do $(XGETTEXT) -C -k_ -j -o $(BUILDDIR)/imessages.po $$i ; \
	done

clean:
	$(RM) $(IDLFILES) $(INTERFACES) $(CPPFILES)

doc:
	if [ ! -d ../doc/classes ] ; then mkdir ../doc/classes ; fi
# scandoc isn't clever enough for Vadim's coding
	-$(DOCHTML) *.h

.PHONY: clean doc all install msgcat interface
