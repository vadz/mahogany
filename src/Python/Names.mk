# Names.mk for Python directory
# $Id$

# only need to do this if we're including Python support
ifdef USE_PYTHON

POBJS	:= $(patsubst .src/%.py-swig,%.py,$(wildcard .src/Python/*.py-swig))
POBJS	:= $(filter-out Python/MTest.py, $(POBJS))
MSRC	:= Python/InitPython.cpp Python/PythonHelp.cpp Python/PythonDll.cpp

MOBJS	+= $(POBJS:.py=.o) $(MSRC:.cpp=.o)
MSGSRC	+= $(MSRC)

# the rule below is used to build .o file from the SWIG .i source
#
# the goal of the sed line is to make *.o file to depend on the *.i source,
# not on the intermediate *.cpp generated from it
define M_COMPILE_SWIG
$(CXX) -o $@ $(strip $(M_COMPILE_CXX)) $*.cpp
@rm -f $*.cpp
@f=$(notdir $*); test ! -f $*.d || { sed -e "s,^$$f\.o:,$@:," -e "s,$*.cpp,$<," $*.d >$*.t && rm -f $*.d && mv $*.t $*.d; }
endef

# swiglib.i only exists for global stuff to be compiled in it and for this we
# must define this symbol when compiling it
#
# also see "-c" swig option handling below: it must not be specified for
# swiglib
CXXFLAGS_Python_swiglib_o := -DSWIG_GLOBAL

ifdef SWIG
SWIGFLAGS := -c++ -python -shadow
vpath %.i .src
%.o %.py: %.i
	$(SWIG) -I$(dir $<) $(CPPFLAGS) $(SWIGFLAGS) \
	 $(if $(subst swiglib,,$(notdir $*)),-c) -o $*.cpp $<
	$(M_COMPILE_SWIG)
else
vpath %.cpp-swig .src
%.o: %.cpp-swig
	cp -f $< $*.cpp
	cp -f $*.py-swig $*.py
	$(M_COMPILE_SWIG)
endif

CLEAN	+= Python/swiglib.cpp Python/swiglib.py Python/swiglib_wrap.html \
	$(POBJS) $(POBJS:.py=.cpp) $(POBJS:.py=_wrap.html)

endif	# USE_PYTHON
