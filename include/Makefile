# Makefile for include directory

CWD = classes
HDR = *.h

include ../makeopts

INSTALLTARGET=

all: 

install: $(INSTALLTARGET)

msgcat:
	@touch $(BUILDDIR)/imessages.po
	@for i in *.h gui/*.h adb/*.h ; do $(XGETTEXT) -C -a -j -o $(BUILDDIR)/imessages.po $$i ; done

clean:

dep depend:

doc:
	$(DOCHTML) *.h
	$(DOCTEX) *.h


.PHONY: clean doc doc2 dep all install msgcat

