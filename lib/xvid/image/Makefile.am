SUBDIRS = x86_asm

noinst_LTLIBRARIES = libxvidimage.la
libxvidimage_la_LDFLAGS = -static

AM_CFLAGS= -O3

INCLUDES=-I$(top_builddir)/include

libxvidimage_la_SOURCES = \
	colorspace.c \
	colorspace.h \
	image.c \
	image.h \
	interpolate8x8.c \
	interpolate8x8.h

if USE_MMX
libxvidimage_la_LIBADD = x86_asm/libxvidimagex86.la
endif
