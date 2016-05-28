## Process this file with automake to produce Makefile.in

bin_PROGRAMS= peepd
peepd_SOURCES= \
	Main.c \
	Main.h \
	Parser.c \
	Parser.h \
	Server.c \
	Server.h \
	Engine.c \
	Engine.h \
	EngineQueue.c \
	EngineQueue.h \
	MixerQueue.c \
	MixerQueue.h \
	VoiceMixer.c \
	VoiceMixer.h \
	Thread.c \
	Thread.h \
	Playback.c \
	Playback.h \
	Debug.c \
	Debug.h \
	DeviceSound.c \
	AlsaSound.c \
	Sound.h \
	Copyright.h

EXTRADIST = peep.conf.template

INCLUDES = -I. -I$(srcdir)

etc = /etc

includes:

make-install-dirs:
	-if test '!' -d $(prefix)$(etc); then mkdir -m 775 $(prefix)$(etc); fi

install-data-hook: make-install-dirs
	-@if test '!' -f $(prefix)$(etc)/peep.conf ; then \
	 echo Installing $(prefix)$(etc)/peep.conf ; \
	 $(INSTALL_DATA) $(srcdir)/peep.conf $(prefix)$(etc)/peep.conf ; \
	fi
