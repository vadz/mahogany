# Makefile for src/adb directory

CWD = adb
SRC = AdbFrame.cpp AdbManager.cpp ProvFC.cpp AdbProvider.cpp

ifdef DEBUG
SRC += ProvDummy.cpp
endif

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o)

all: $(TARGETS)

include ../../makeopts
include ../../makerules
