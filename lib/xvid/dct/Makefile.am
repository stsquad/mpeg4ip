SUBDIRS = x86_asm

noinst_LTLIBRARIES = libxviddct.la
libxviddct_la_LDFLAGS = -static

AM_CFLAGS= -O3
INCLUDES=-I$(top_builddir)/include

libxviddct_la_SOURCES = \
	fdct.c \
	fdct.h \
	idct.c \
	idct.h

if USE_MMX
libxviddct_la_LIBADD = x86_asm/libxviddctx86.la
endif
