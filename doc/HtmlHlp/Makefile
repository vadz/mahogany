# Makefile for doc/HtmlHlp
# $Id$

include ../../makeopts

GENERATED = Manual.tex Manual.html Manual.hhc
BACK_TO_SOURCES = $(patsubst %,.src/%,$(GENERATED)) .src/Manual.html

htmlhlp: Manual.hhc

Manual.tex: .src/../Manual.htex
	../../.src/extra/scripts/striphelpids $< > $@

# latex2html options are needed to produce a single HTML file without
# any navigation buttons which we don't need in CHM files anyhow
Manual.html: Manual.tex
	@if [ ! -d Manual ]; then mkdir Manual; fi; \
	$(LATEX2HTML) -local_icons -split 0 -info 0 -verbosity 0 -no_navigation \
            -dir Manual Manual.tex; \
	mv Manual/Manual.html .

Manual.hhc: Manual.html
	$(PERL) -w .src/makehhc.pl

install: htmlhlp
	$(INSTALL_DATA) Manual.hhc .src/Manual.hhp Manual.html $(DOCDIR)
	@# this file must exist or HTML help controller complains
	touch $(DOCDIR)/Manual.hhk

$(BACK_TO_SOURCES): .src/%: %
	cp $< $@

update: $(BACK_TO_SOURCES)

clean:
	rm -rf Manual Manual.tex Manual.html

.PHONY: htmlhlp update clean
