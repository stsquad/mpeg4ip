
## Makefile.am for the SDL audio library

noinst_LTLIBRARIES = libaudio.la

EXTRA_SUBDIRS = aix amigaos beos irix linux macos openbsd solaris win32

SUBDIRS = @AUDIO_ARCH@

DIST_SUBDIRS = $(EXTRA_SUBDIRS)

# Include the architecture-independent sources
COMMON_SRCS =			\
	SDL_audio.c		\
	SDL_audio_c.h		\
	SDL_audiocvt.c		\
	SDL_audiomem.c		\
	SDL_audiomem.h		\
	SDL_mixer.c		\
	SDL_sysaudio.h		\
	SDL_wave.c		\
	SDL_wave.h

libaudio_la_SOURCES = $(COMMON_SRCS)
libaudio_la_LIBADD = @AUDIO_ARCH_LIB@
