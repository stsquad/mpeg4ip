
## Makefile.am for the SDL OpenBSD audio subsystem

noinst_LTLIBRARIES = libaudio_arch.la

if TARGET_OPENBSD
ARCH_SRCS = SDL_audiodev.c SDL_audiodev_c.h \
            SDL_lowaudio.h SDL_sysaudio.c \
            SDL_esdaudio.c SDL_esdaudio.h
else
ARCH_SRCS = 
endif

libaudio_arch_la_SOURCES = $(ARCH_SRCS)

# All of the sources come from other architecture directories
DISTCLEANFILES = $(ARCH_SRCS)
