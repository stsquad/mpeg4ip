
## Makefile.am for the SDL Win32 audio subsystem

noinst_LTLIBRARIES = libaudio_arch.la

DIB_SRCS = SDL_dibaudio.h SDL_dibaudio.c
DX5_SRCS = SDL_dx5audio.h SDL_dx5audio.c

if USE_DIRECTX
ARCH_SRCS = $(DIB_SRCS) $(DX5_SRCS)
EXTRA_SRCS =
else
ARCH_SRCS = $(DIB_SRCS)
endif
EXTRA_DIST = $(DX5_SRCS)

libaudio_arch_la_SOURCES = $(ARCH_SRCS)
