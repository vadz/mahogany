# Names.mk for Python directory
# $Id$

# only need to do this if we're including Python support
ifdef USE_PYTHON

IFACE_DIR := .src/../include/interface

IFILES	:= $(filter-out swigcmn.i, $(notdir $(wildcard $(IFACE_DIR)/*.i)))
MSRC	:= Python/InitPython.cpp Python/PythonHelp.cpp

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

ifdef SWIG

# this command runs swig to generate .cpp file from an .i one and then replaces
# #include Python.h in the generated C++ code with #include MPython.h which we
# need for dynamic Python linking to work
#
# NB: current versions of swig (at least from 1.3.29 to 1.3.33) are confused by
#     WXDLLIMPEXP_FWD_XXX in declarations, so we need to undefine them
define create_cpp
	$(SWIG) -DWXDLLIMPEXP_FWD_BASE="" -DWXDLLIMPEXP_FWD_CORE="" \
	    -I$(IFACE_DIR) $(CPPFLAGS) $(SWIGFLAGS) -o $*.cpp.tmp $< && \
	    sed -e 's/Python\.h/MPython.h/' $*.cpp.tmp > $(@:.o=.cpp) && \
		rm $*.cpp.tmp
endef

SWIGFLAGS := -c++ -python
vpath %.i $(IFACE_DIR)

Python/%.o Python/%.py: %.i
	$(create_cpp)
	mv -f $*.py Python
	$(M_COMPILE_SWIG)
	@rm -f Python/%.cpp

# define rule for copying the swig-generated files back to the source tree:
# this is ugly but necessary as we want to store them in the cvs to be able to
# build M without swig
.src/Python/%.cpp-swig .src/Python/%.py-swig: %.i
	$(create_cpp)
	mv -f $*.py .src/Python/$*.py-swig

.src/Python/Mswigpyrun.h:
	$(SWIG) -python -external-runtime $@

swig-update: $(patsubst %.i,.src/Python/%.cpp-swig,$(IFILES)) .src/Python/Mswigpyrun.h

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
