if USE_MMX
SUBDIRS = x86_asm
endif

noinst_LTLIBRARIES = libxvidutils.la
libxvidutils_la_LDFLAGS = -static

AM_CFLAGS= -O3

libxvidutils_la_SOURCES = \
	emms.c \
	emms.h \
	mbfunctions.h \
	mbtransquant.c \
	mem_align.c \
	mem_align.h \
	mem_transfer.c \
	mem_transfer.h \
	ratecontrol.c \
	ratecontrol.h \
	timer.c \
	timer.h

if USE_MMX
libxvidutils_la_LIBADD = x86_asm/libxvidutilsx86.la
endif
