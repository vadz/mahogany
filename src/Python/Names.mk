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
# want to keep .cpp around, so erase it after compiling and also make the .o
# depend on the real source file and not on this intermediate .cpp one
define M_COMPILE_SWIG
$(CXX) -o $@ $(strip $(M_COMPILE_CXX)) $(@:.o=.cpp)
@test -f $(@:.o=.d) && { sed -e "s,$(@:.o=.cpp),$<," $(@:.o=.d) >$(@:.o=.d)2 && rm -f $(@:.o=.d) && mv $(@:.o=.d)2 $(@:.o=.d); }
@rm -f $(@:.o=.cpp)
endef

# swiglib.i only exists for global stuff to be compiled in it and for this we
# must define this symbol when compiling it
#
# also see "-c" swig option handling below: it must not be specified for
# swiglib
CXXFLAGS_Python_swiglib_o := -DSWIG_GLOBAL

ifdef SWIG

# this command replaces #include Python.h in the generated C++ code with
# #include MPython.h which we need for dynamic Python linking to work
#
# further, newer versions of swig (1.3.21) now automatically rename SWIG_xxx
# functions to SWIG_Python_xxx which is very nice but incompatible with the
# old versions because we have to also declare these functions ourselves as
# they're not declared in any header -- and as we don't have an easy way to
# check how the functionsare called we simply disable the #define's which do
# the renaming in swig-generated code
define create_cpp
	$(SWIG) -I$(IFACE_DIR) $(CPPFLAGS) $(SWIGFLAGS) \
	 $(if $(subst swiglib,,$*),-c) -o $(@:.o=.cpp) $< && \
	sed -e 's/Python\.h/MPython.h/' \
		 -e '/^#define SWIG_\(\w\+\) \+SWIG_Python_\1/d' \
		 -e '/^#define SWIG_\w\+(.*\\$$/,/^ \+SWIG_Python_\w\+(/d' \
		 -e 's/SWIG_Python_/SWIG_/g' $(@:.o=.cpp) \
			> $(@:.o=.cpp).new && mv $(@:.o=.cpp).new $(@:.o=.cpp)
endef

SWIGFLAGS := -c++ -python -shadow
vpath %.i $(IFACE_DIR)
Python/%.o Python/%.py: %.i
	$(create_cpp)
	$(M_COMPILE_SWIG)

# define rule for copying the swig-generated files back to the source tree:
# this is ugly but necessary as we want to store them in the cvs to be able to
# build M without swig
.src/Python/%.cpp-swig .src/Python/%.py-swig: %.i
	$(create_cpp)
	mv -f $*.cpp .src/Python/$*.cpp-swig
	cp -f $*.py .src/Python/$*.py-swig

swig-update: $(patsubst %.i,.src/Python/%.cpp-swig,$(IFILES))

else # !SWIG

vpath %.cpp-swig .src
%.o: %.cpp-swig
	cp -f $< $*.cpp
	cp -f .src/$*.py-swig $*.py
	$(M_COMPILE_SWIG)
	@rm $*.py

endif # SWIG/!SWIG

#CLEAN	+= Python/swiglib.cpp Python/swiglib.py Python/swiglib_wrap.html \
#	$(POBJS) $(POBJS:.py=.cpp) $(POBJS:.py=_wrap.html)

endif	# USE_PYTHON
