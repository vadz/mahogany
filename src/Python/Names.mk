# Names.mk for Python directory
# $Id$

# only need to do this if we're including Python support
ifdef USE_PYTHON

IFACE_DIR := .src/../include/interface

IFILES	:= $(filter-out swigcmn.i, $(notdir $(wildcard $(IFACE_DIR)/*.i)))
MSRC	:= Python/InitPython.cpp Python/PythonHelp.cpp Python/PythonDll.cpp

MOBJS	+= $(patsubst %.i,Python/%.o,$(IFILES)) $(MSRC:.cpp=.o)
MSGSRC	+= $(MSRC)

# build .o file from a temporary .cpp generated from SWIG .i source: we don't
# want to keep .cpp around, so erase it after compiling
define M_COMPILE_SWIG
$(CXX) -o $@ $(strip $(M_COMPILE_CXX)) $(@:.o=.cpp)
@rm -f $(@:.o=.cpp)
endef

# swiglib.i only exists for global stuff to be compiled in it and for this we
# must define this symbol when compiling it
#
# also see "-c" swig option handling below: it must not be specified for
# swiglib
CXXFLAGS_Python_swiglib_o := -DSWIG_GLOBAL

ifdef SWIG
# the goal of this command is to make *.o file to depend on the *.i source,
# not on the intermediate *.cpp generated from it
define adjust_dep
@cd Python && \
test ! -f $*.d || { sed -e "s,$(@:.o=.cpp),$<," $*.d >$*.d2 && rm -f $*.d && mv $*.d2 $*.d; }
endef

SWIGFLAGS := -c++ -python -shadow
vpath %.i $(IFACE_DIR)
Python/%.o Python/%.py: %.i
	cd Python && \
	$(SWIG) -I../$(IFACE_DIR) $(CPPFLAGS) $(SWIGFLAGS) \
	 $(if $(subst swiglib,,$*),-c) -o $*.cpp $<
	$(M_COMPILE_SWIG)
	$(adjust_dep)
else
vpath %.cpp-swig .src
%.o: %.cpp-swig
	cp -f $< $*.cpp
	cp -f $*.py-swig $*.py
	$(M_COMPILE_SWIG)
endif

#CLEAN	+= Python/swiglib.cpp Python/swiglib.py Python/swiglib_wrap.html \
#	$(POBJS) $(POBJS:.py=.cpp) $(POBJS:.py=_wrap.html)

endif	# USE_PYTHON
