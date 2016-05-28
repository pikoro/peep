## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.0 foreign dist-zip

PACKAGE = peep
VERSION = 0.4.2

SUBDIRS = server

EXTRA_DIST= \
	README \
	COPYING \
	INSTALL \
	ChangeLog \
	TODO \
	server/peep.conf.template \
	gen_conf

DISTCLEANFILES= server/peep.conf

# Defines for the sound data
share = $(prefix)/share/$(PACKAGE)/$(sounddir)
sounddir = sounds

includes:

make-install-dirs:
	-if test '!' -d $(share); then mkdir -p $(share); fi

install-data-hook: make-install-dirs
	@echo
	@echo ==========================================================
	@echo Installing shared sound data in $(prefix)/share/$(PACKAGE).
	@echo Use these sounds when configuring your peep.conf.
	@echo
	-for thing in `find $(sounddir)`; do \
		if test -d $$thing && test ! -d $(prefix)/share/$(PACKAGE)/$$thing; then \
			mkdir -m 755 $(prefix)/share/$(PACKAGE)/$$thing; \
		elif test -f $$thing && test ! -e $(prefix)/share/$(PACKAGE)/$$thing; then \
			$(INSTALL_DATA) $$thing $(prefix)/share/$(PACKAGE)/$$thing; \
		fi \
	done

uninstall:
	@echo Removing sound data dir: $(share);
	-if test -d $(share); then rm -rf $(prefix)/share/$(PACKAGE); fi

dist-hook:
	mkdir $(distdir)/client
	cp -r -p client/* $(distdir)/client
	mkdir $(distdir)/docs
	cp -r -p docs/* $(distdir)/docs
