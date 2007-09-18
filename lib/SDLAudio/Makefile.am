# The top-level input Makefile for SDL

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

## Any directories that you want built and installed should go here.
SUBDIRS = src include 

## Any directories you want a part of the distribution should be listed
## here, as well as have a Makefile generated at the end of configure.in
##
## This only works for subdirectories one level deep.
DIST_SUBDIRS = $(SUBDIRS)

# SDL runtime configuration script
# All the rest of the distributed files
EXTRA_DIST =		\
	COPYING		\
	CREDITS		\
	README		\
	README.AmigaOS	\
	README.CVS	\
	README.DC	\
	README.Epoc	\
	README.MacOS	\
	README.MacOSX	\
	README.MiNT	\
	README.NanoX	\
	README.PicoGUI	\
	README.QNX	\
	README.Qtopia	\
	README.WinCE	\
	README-SDL.txt	\
	WhatsNew	\
	SDLaudio.dsp SDLaudio.vcproj    \
	sdl.m4

# M4 macro file for inclusion with autoconf
m4datadir = $(datadir)/aclocal

