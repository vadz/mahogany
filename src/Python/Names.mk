# Names.mk for Python directory
# $Id$

# only need to do this if we're including Python support
ifdef USE_PYTHON

POBJS	:= $(patsubst .src/%.py-swig,%.py,$(wildcard .src/Python/*.py-swig))
POBJS	:= $(filter-out Python/MTest.py, $(POBJS))
MSRC	:= Python/InitPython.cpp Python/PythonHelp.cpp

MOBJS	+= $(POBJS:.py=.o) $(MSRC:.cpp=.o)
MSGSRC	+= $(MSRC)

# common compilation steps
define M_COMPILE_SWIG
$(CXX) -o $@ $(strip $(M_COMPILE_CXX)) -DSWIG_NOINCLUDE $*.cpp
@rm -f $*.cpp
@f=$(notdir $*); test ! -f $$f.d || { sed -e "s,^$$f\.o:,$@:," -e "s,$*.cpp,$<," $$f.d >$*.t && rm -f $$f.d && mv $*.t $*.d; }
endef

define M_COMPILE_SWIGLIB
$(CXX) -o $@ $(strip $(M_COMPILE_CXX)) -DSWIG_GLOBAL $*.cpp
@rm -f $*.cpp
@f=$(notdir $*); test ! -f $$f.d || { sed -e "s,^$$f\.o:,$@:," -e "s,$*.cpp,$<," $$f.d >$*.t && rm -f $$f.d && mv $*.t $*.d; }
endef

ifdef SWIG
SWIGFLAGS := -c++ -python -shadow
vpath %.i .src
%.o %.py: %.i
	@rm -f $*.py
	$(SWIG) -I$(dir $<) $(SWIGFLAGS) -c -o $*.cpp $<
	$(M_COMPILE_SWIG)
Python/swiglib.o: Python/swiglib.i
	$(SWIG) -I$(dir $<) $(SWIGFLAGS) -o $*.cpp $<
	$(M_COMPILE_SWIGLIB)
else
vpath %.cpp-swig .src
%.o: %.cpp-swig
	cp -f $< $*.cpp
	cp -f $(basename $<).py-swig $*.py
	$(M_COMPILE_SWIG)
Python/swiglib.o: Python/swiglib.cpp-swig
	cp -f $< $*.cpp
	$(M_COMPILE_SWIGLIB)
endif

INSTALL_DIRS += Python

CLEAN	+= Python/swiglib.cpp Python/swiglib.py Python/swiglib_wrap.html \
	$(POBJS) $(POBJS:.py=.cpp) $(POBJS:.py=_wrap.html)

endif	# USE_PYTHON
