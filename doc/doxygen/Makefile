# Makefile for doc/doxygen directory
#
# This makefile is used from the source directory (i.e. it can be used without
# running configure) to create the documentation for Mahogany API using doxygen
#
# Author:  Thomas Finneid
# Created: 21.12.01
# Version: $Id$

all: dox.complete

# document only non-private non-static members, omit some less useful sections
dox.simple:
	$(MAKE) dox.phony TYPE=simple

# document everything
dox.complete:
	$(MAKE) dox.phony TYPE=complete

# document everything and generate the class diagram (requires dot tool)
dox.diagram:
	$(MAKE) dox.phony TYPE=diagram

dox.phony:
	cd $(SOURCEDIR); \
	doxygen doxyfile.$(TYPE); \
	echo ""; \
	echo "The class documentation is available in M-API/html.$(TYPE)/index.html"; \
	echo ""; \

