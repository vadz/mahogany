# Makefile for doc/HtmlHlp
# $Id$

include ../../makeopts

GENERATED = Manual.tex Manual.html Manual.hhc
BACK_TO_SOURCES = .src/Manual.hhc .src/Manual.html

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
	$(INSTALL_DATA) .src/Manual.chm Manual.hhc .src/Manual.hhk .src/Manual.hhp Manual.html $(DOCDIR)

.src/Manual.html: Manual.html
	cp $< $@

.src/Manual.hhc: Manual.hhc
	cp $< $@

update: .src/Manual.hhc .src/Manual.html

clean:
	rm -rf Manual Manual.tex Manual.html

.PHONY: htmlhlp update clean
