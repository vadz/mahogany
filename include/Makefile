# Makefile for include directory

CWD = classes
HDR = *.h

include ../makeopts

INSTALLTARGET=

ifdef DOCHTML
INSTALLTARGET += doc
endif

all: 

install: $(INSTALLTARGET)

msgcat:
	@touch $(BUILDDIR)/imessages.po
	@for i in *.h gui/*.h adb/*.h ; do $(XGETTEXT) -C -a -j -o $(BUILDDIR)/imessages.po $$i ; done
	@msgmerge $(BUILDDIR)/imessages.po $(BUILDDIR)/messages.po -o tmp.po && mv tmp.po $(BUILDDIR)/messages.po 
clean:

dep depend:

doc:
	$(DOCHTML) *.h
#	$(DOCTEX) *.h

doc2:
	for i in *.h ; do $(DXX) -f -k -u -k $(DXXTBL) -H -t -o $(BUILDDIR)/doc/classes/$$i.tex $$i ; done

.PHONY: clean doc doc2 dep all install msgcat

