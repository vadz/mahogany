# Names.mk for wx directory
# $Id$

include $(wildcard .src/wx/*/Names.mk) $(wildcard $(patsubst .src/%/Names.mk,%/*.d,$(wildcard .src/wx/*/Names.mk)))

# need to explicitly specify the path containing vcc.h when compiling in a
# separate build dir
CFLAGS_wx_vcard_vcc_o := -I.src/wx/vcard
