if USE_MMX
SUBDIRS = x86_asm
endif

noinst_LTLIBRARIES = libxviddct.la
libxviddct_la_LDFLAGS = -static

AM_CFLAGS= -O3

libxviddct_la_SOURCES = \
	fdct.c \
	fdct.h \
	idct.c \
	idct.h

if USE_MMX
libxviddct_la_LIBADD = x86_asm/libxviddctx86.la
endif
