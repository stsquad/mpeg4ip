if USE_MMX
SUBDIRS = x86_asm
endif

noinst_LTLIBRARIES = libxvidbitstream.la
libxvidbitstream_la_LDFLAGS = -static

AM_CFLAGS= -O3

libxvidbitstream_la_SOURCES = \
	bitstream.c \
	bitstream.h \
	cbp.c \
	cbp.h \
	mbcoding.c \
	mbcoding.h \
	vlc_codes.h \
	zigzag.h

if USE_MMX
libxvidbitstream_la_LIBADD = x86_asm/libxvidbitstreamx86.la
endif
