# Names.mk for wx directory
# $Id$

include $(wildcard .src/wx/*/Names.mk) $(wildcard $(patsubst .src/%/Names.mk,%/*.d,$(wildcard .src/wx/*/Names.mk)))
