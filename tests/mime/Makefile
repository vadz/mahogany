WX_CONFIG := wx-config

ifndef top_builddir
$(error Define top_builddir to point to build directory on make command line)
endif

top_srcdir := ../..

CXXFLAGS := -I$(top_srcdir)/include `$(WX_CONFIG) --cxxflags` -g

all: decode

decode: decode.o $(top_builddir)/src/mail/MimeDecode.o
	`$(WX_CONFIG) --cxx` -o $@ $^ `$(WX_CONFIG) --libs`

decode.o: decode.cpp

$(top_builddir)/src/mail/MimeDecode.o: $(top_srcdir)/src/mail/MimeDecode.cpp
	$(MAKE) -C $(top_builddir)/src mail/MimeDecode.o

clean:
	$(RM) decode.o decode

.PHONY: all clean
