# Makefile for include directory

CWD = classes
HDR = *.h

MID:=$(wildcard *.mid)
M4DEF:=$(wildcard *.m4)
IDLFILES:=$(MID:.mid=.idl)
INTERFACES:=$(MID:.mid=.h)
CPPFILES:=$(MID:.mid=.cpp)

include ../makeopts

INSTALLTARGET=



all: interface

interface: $(IDLFILES) $(INTERFACES) $(CPPFILES)

install: $(INSTALLTARGET)

$(IDLFILES) $(INTERFACES) $(CPPFILES): $(MID) $(M4DEF)

msgcat:
	@touch $(BUILDDIR)/imessages.po
	@for i in *.h gui/*.h adb/*.h ; do $(XGETTEXT) -C -k_ -j -o $(BUILDDIR)/imessages.po $$i ; done

clean:
	$(RM) $(IDLFILES) $(INTERFACES) $(CPPFILES)

dep depend:

doc:
	$(DOCHTML) *.h
	$(DOCTEX) *.h

scandoc:
	$(SCANDOCHTML) *.h

.PHONY: clean doc doc2 dep all install msgcat scandoc interface

%.h: %.mid
	$(M4) -DM4FILE=mid2h.m4 >$(ODIR)/$@ $<

%.cpp: %.mid
	$(M4) -DM4FILE=mid2cpp.m4 >$(ODIR)/$@ $<

%.idl: %.mid
	$(M4) -DM4FILE=mid2idl.m4 >$(ODIR)/$@ $<

