# Makefile for include directory
#
# Here we produce the files MInterface.cpp and MInterface.h in the build
# directory from MInterface.mid using m4 and we also have an update target to
# copy these generated files back to the source directory from where they can
# be checked in the cvs

MID:=$(wildcard .src/*.mid)
M4DEF:=$(wildcard .src/mid2*.m4)
IDLFILES:=$(patsubst .src/%.mid,%.idl,$(MID))
INTERFACES:=$(IDLFILES:.idl=.h)
CPPFILES:=$(IDLFILES:.idl=.cpp)
SRC_INTERFACES:=$(patsubst %,.src/%,$(INTERFACES:.h=.h.m4))
SRC_CPPFILES:=$(patsubst %,.src/%,$(CPPFILES:.cpp=.cpp.m4))

include ../makeopts

ifdef M4
vpath %.mid .src

%.h: %.mid
	$(M4) -DM4FILE=.src/mid2h.m4 >$@ $<

%.cpp: %.mid
	$(M4) -DM4FILE=.src/mid2cpp.m4 >$@ $<

%.idl: %.mid
	$(M4) -DM4FILE=.src/mid2idl.m4 >$@ $<
else
vpath %.m4 .src

%.cpp: %.cpp.m4
	cp $< $@

%.h: %.h.m4
	cp $< $@

%.idl: %.idl.m4
	cp $< $@
endif

all:	interface
	@true

interface: $(IDLFILES) $(INTERFACES) $(CPPFILES)

install:
	@true

ifdef M4
# this target updates the .*.m4 files in the source tree from the regenerated
# versions
%.cpp.m4: %.cpp
	cp $< $@

%.h.m4: %.h
	cp $< $@

update: $(SRC_INTERFACES) $(SRC_CPPFILES)
endif

$(IDLFILES) $(INTERFACES) $(CPPFILES): $(MID) $(M4DEF)

msgcat:
	@touch $(BUILDDIR)/messages.po
	cd .src; exec $(XGETTEXT) -C -k_ -j -o $(BUILDDIR)/messages.po *.h gui/*.h adb/*.h

clean:
	$(RM) $(IDLFILES) $(INTERFACES) $(CPPFILES)

doc:
	if [ ! -d ../doc/classes ] ; then mkdir ../doc/classes ; fi
# scandoc isn't clever enough for Vadim's coding
	-$(DOCHTML) *.h

.PHONY: clean doc all install msgcat interface
