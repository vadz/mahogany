# Makefile for include directory

CWD = classes
HDR = *.h

MID:=$(wildcard *.mid)
M4DEF:=$(wildcard *.m4)
IDLFILES:=$(MID:.mid=.idl)
INTERFACES:=$(MID:.mid=.h)

include ../makeopts

INSTALLTARGET=



all: $(IDLFILES) $(INTERFACES)

install: $(INSTALLTARGET)

$(IDLFILES) $(INTERFACES): $(MID) $(MIDDEF)

msgcat:
	@touch $(BUILDDIR)/imessages.po
	@for i in *.h gui/*.h adb/*.h ; do $(XGETTEXT) -C -k_ -j -o $(BUILDDIR)/imessages.po $$i ; done

clean:
	$(RM) $(IDLFILES) $(INTERFACES)

dep depend:

doc:
	$(DOCHTML) *.h
	$(DOCTEX) *.h

scandoc:
	$(SCANDOCHTML) *.h

.PHONY: clean doc doc2 dep all install msgcat scandoc

%.h: %.mid
	$(M4) -DINCLUDE=mid2h.m4 >$(ODIR)/$@ $<

%.idl: %.mid
	$(M4) -DINCLUDE=mid2idl.m4 >$(ODIR)/$@ $<

