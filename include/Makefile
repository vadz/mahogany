# Makefile for include directory

CWD = classes
HDR = *.h

all: 

install: doc

include ../makeopts

clean:

dep depend:

doc:
	$(DOCHTML) *.h
	$(DOCTEX) *.h

#$(MKDIR) $(DOCDIR)/$(CWD)
#$(DXX) $(DXXHFLAGS) docxx.h #$(HDR)
#$(DXX) $(DXXTFLAGS) docxx.h #$(HDR)

doc2:
	for i in *.h ; do $(DXX) -f -k -u -k $(DXXTBL) -H -t -o $(DOCDIR)/$$i.tex $$i ; done

.PHONY: clean doc doc2 dep all install

