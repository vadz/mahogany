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

clean:

dep depend:

doc:
	$(DOCHTML) *.h
	$(DOCTEX) *.h

#$(MKDIR) $(DOCDIR)/$(CWD)
#$(DXX) $(DXXHFLAGS) docxx.h #$(HDR)
#$(DXX) $(DXXTFLAGS) docxx.h #$(HDR)

doc2:
	for i in *.h ; do $(DXX) -f -k -u -k $(DXXTBL) -H -t -o $(BUILDDIR)/doc/classes/$$i.tex $$i ; done

.PHONY: clean doc doc2 dep all install

