# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# On OS X, install the SDLMain.nib.
# We use an ugly hack to force the creation of the
# destination dir, maybe somebody with more automake
# experience knows how to do this properly?


SHELL = /bin/sh

srcdir = .
top_srcdir = ../../..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/SDL
pkglibdir = $(libdir)/SDL
pkgincludedir = $(includedir)/SDL

top_builddir = ../../..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i386-unknown-freebsd4.3
host_triplet = i386-unknown-freebsd4.3
ARCH = freebsd
ARTSCCONFIG = 
AS = 
ASFLAGS = 
AUDIO_DRIVERS =  disk/libaudio_disk.la dsp/libaudio_dsp.la dma/libaudio_dma.la esd/libaudio_esd.la
AUDIO_SUBDIRS =  disk dsp dma esd
CC = gcc
CDROM_DRIVERS =  freebsd/libcdrom_freebsd.la
CDROM_SUBDIRS =  freebsd
CXX = c++
DIRECTFB_CFLAGS = 
DIRECTFB_LIBS = 
DLLTOOL = 
ESD_CFLAGS = -I/usr/local/include
ESD_CONFIG = /usr/local/bin/esd-config
ESD_LIBS = -L/usr/local/lib -lesd -laudiofile -lm
JOYSTICK_DRIVERS =  dummy/libjoystick_dummy.la
JOYSTICK_SUBDIRS =  dummy
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
LT_AGE = 0
LT_CURRENT = 0
LT_RELEASE = 1.2
LT_REVISION = 3
MAKEINFO = makeinfo
NASM = 
NASMFLAGS = 
OBJC = ???
OBJCDEPMODE = 
OBJCFLAGS = 
OBJDUMP = 
PACKAGE = SDL
PKG_CONFIG = 
PTH_CONFIG = 
RANLIB = ranlib
SDL_BINARY_AGE = 3
SDL_CFLAGS =  -D_REENTRANT -D_THREAD_SAFE
SDL_EXTRADIRS =  audio video events joystick cdrom thread timer endian file
SDL_EXTRALIBS =  audio/libaudio.la video/libvideo.la events/libevents.la joystick/libjoystick.la cdrom/libcdrom.la thread/libthread.la timer/libtimer.la endian/libendian.la file/libfile.la
SDL_INTERFACE_AGE = 3
SDL_LIBS = -lSDL -pthread
SDL_MAJOR_VERSION = 1
SDL_MICRO_VERSION = 3
SDL_MINOR_VERSION = 2
SDL_RLD_FLAGS = -Wl,-rpath,${exec_prefix}/lib
SDL_VERSION = 1.2.3
SYSTEM_LIBS =  -lm -L/usr/local/lib -lesd -laudiofile -lm  -L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga -lXv -lXinerama
VERSION = 1.2.3
VIDEO_DRIVERS =  dummy/libvideo_null.la x11/libvideo_x11.la dga/libvideo_dga.la
VIDEO_SUBDIRS =  dummy x11 dga

#hackhacknibdatadir = $(nibdatadir)/SDLMain.nib
#hackhacknibdata_DATA = info.nib

#nibdatadir = $(datadir)/sdl
#nibdata_DATA = 	SDLMain.nib/classes.nib		SDLMain.nib/info.nib		SDLMain.nib/objects.nib		Info.plist

EXTRA_DIST = SDLMain.h SDLMain.m info.nib
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_CLEAN_FILES =  Info.plist
DATA =  $(hackhacknibdata_DATA) $(nibdata_DATA)

DIST_COMMON =  Info.plist.in Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
all: all-redirect
.SUFFIXES:
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --foreign src/main/macosx/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

Info.plist: $(top_builddir)/config.status Info.plist.in
	cd $(top_builddir) && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

install-hackhacknibdataDATA: $(hackhacknibdata_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(hackhacknibdatadir)
	@list='$(hackhacknibdata_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(hackhacknibdatadir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(hackhacknibdatadir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(hackhacknibdatadir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(hackhacknibdatadir)/$$p; \
	  fi; fi; \
	done

uninstall-hackhacknibdataDATA:
	@$(NORMAL_UNINSTALL)
	list='$(hackhacknibdata_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(hackhacknibdatadir)/$$p; \
	done

install-nibdataDATA: $(nibdata_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(nibdatadir)
	@list='$(nibdata_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(nibdatadir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(nibdatadir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(nibdatadir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(nibdatadir)/$$p; \
	  fi; fi; \
	done

uninstall-nibdataDATA:
	@$(NORMAL_UNINSTALL)
	list='$(nibdata_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(nibdatadir)/$$p; \
	done
tags: TAGS
TAGS:


distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = src/main/macosx

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --foreign src/main/macosx/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
	$(MAKE) $(AM_MAKEFLAGS) top_distdir="$(top_distdir)" distdir="$(distdir)" dist-hook
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am:
install-exec: install-exec-am

install-data-am: install-hackhacknibdataDATA install-nibdataDATA
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-hackhacknibdataDATA uninstall-nibdataDATA
uninstall: uninstall-am
all-am: Makefile $(DATA)
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(hackhacknibdatadir) \
		$(DESTDIR)$(nibdatadir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-generic clean-am
	-rm -f libtool

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: uninstall-hackhacknibdataDATA install-hackhacknibdataDATA \
uninstall-nibdataDATA install-nibdataDATA tags distdir info-am info \
dvi-am dvi check check-am installcheck-am installcheck install-exec-am \
install-exec install-data-am install-data install-am install \
uninstall-am uninstall all-redirect all-am all installdirs \
mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


# The nib and exports directories need to be copied into place
# when building a distribution.
dist-hook:
	cp -rp SDLMain.nib $(distdir)
	cp -rp exports $(distdir)
	(cd $(distdir) && rm -rf `find . -name CVS`)

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
