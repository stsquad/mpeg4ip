SUBDIRS = x86_asm ppc_asm

noinst_LTLIBRARIES = libxvidbitstream.la
libxvidbitstream_la_LDFLAGS = -static

AM_CFLAGS= -O3
INCLUDES=-I$(top_builddir)/include

libxvidbitstream_la_SOURCES = \
	bitstream.c \
	bitstream.h \
	cbp.c \
	cbp.h \
	h263.h \
	h263.c \
	mbcoding.c \
	mbcoding.h \
	vlc_codes.h \
	zigzag.h

if USE_MMX
XVIDlibs = x86_asm/libxvidbitstreamx86.la
else
if USE_PPC
XVIDlibs = ppc_asm/libxvidbitstreamppc.la
else
XVIDlibs = 
endif
endif
libxvidbitstream_la_LIBADD = $(XVIDlibs)
