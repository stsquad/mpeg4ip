
## Makefile.am for the SDL joystick library

noinst_LTLIBRARIES = libjoystick.la

ARCH_SUBDIRS = dummy beos linux macos win32

# Include the architecture-independent sources
COMMON_SRCS =			\
	SDL_joystick.c		\
	SDL_joystick_c.h	\
	SDL_sysjoystick.h

# Include the architecture-specific sources
if TARGET_BEOS
ARCH_SRCS = SDL_bejoystick.cc
else
if TARGET_WIN32
ARCH_SRCS = SDL_mmjoystick.c
else
ARCH_SRCS = SDL_sysjoystick.c
endif
endif

libjoystick_la_SOURCES = $(COMMON_SRCS) $(ARCH_SRCS)

## Let automake know that it shouldn't distribute linked sources
BUILT_SOURCES = $(ARCH_SRCS)

## Let automake know that it should remove these for distribution
DISTCLEANFILES = $(ARCH_SRCS)

# The architecture specific directories need to be copied into place
# when building a distribution.
dist-hook:
	(cd $(distdir) && rm -f $(BUILT_SOURCES))
	cp -rp $(ARCH_SUBDIRS) $(distdir)
	(cd $(distdir) && rm -rf `find . -name CVS`)
