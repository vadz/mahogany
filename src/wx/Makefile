# Makefile for src/wx directory and all its subdirectories

CWD = wx
SRC = common/mimetype.cpp generic/persctrl.cpp
HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o)

all: $(TARGETS)

include ../../makeopts
include ../../makerules
include .depend

