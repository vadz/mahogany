# Makefile for doc/HtmlHlp
# $Id$

include ../../makeopts

htmlhlp: .src/Manual.hhc

.src/Manual.tex: .src/../Manual.htex
	../../.src/extra/scripts/striphelpids $< > $@

# latex2html options are needed to produce a single HTML file without
# any navigation buttons which we don't need in CHM files anyhow
.src/Manual.html: .src/Manual.tex
	@cd .src; \
	if [ ! -d Manual ]; then mkdir Manual; fi; \
	$(LATEX2HTML) -local_icons -split 0 -info 0 -verbosity 0 -no_navigation \
            -dir Manual Manual.tex; \
	mv Manual/Manual.html .

.src/Manual.hhc: .src/Manual.html
	cd .src && $(PERL) -w makehhc.pl

clean:
	rm -rf .src/Manual .src/Manual.tex .src/Manual.html

.PHONY: htmlhlp clean
