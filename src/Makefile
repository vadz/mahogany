# Makefile for src directory
# $Id$
#
# $Log$
# Revision 1.9  1998/06/22 15:17:00  KB
# support for "make static", fixed duplicate main()
#
# Revision 1.8  1998/06/08 08:19:04  KB
# Fixed makefiles for wxtab/python. Made Python work with new MAppBase.
#
# Revision 1.7  1998/05/19 17:02:42  KB
# several small bugfixes
#
# Revision 1.6  1998/05/18 17:48:27  KB
# more list<>->kbList changes, fixes for wxXt, improved makefiles
#
# Revision 1.5  1998/05/02 18:31:37  KB
# Python integration works.
#
#

CWD = src
SUB_DIRS = util classes gui mail
MOBJS=util/*.o classes/*.o gui/*.o  mail/*.o 

include ../makeopts

ifdef USE_PYTHON
SUB_DIRS += Python
MOBJS += Python/*.o
endif

all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(LINK) -o M $(MOBJS) $(LIBS)
#	$(CXX) -shared $(MOBJS) -lswigpy $(NONPLIBS) -o libM.so
#	$(LINK) -o M -L. -lM $(SLIBS)

static:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done
	$(LINK) -o M.static $(LDFLAGS) $(MOBJS) $(SLIBS) 

doc:
	$(MAKE) -C ../include $@

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core 

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done


.PHONY: all dep clean doc static allclean



