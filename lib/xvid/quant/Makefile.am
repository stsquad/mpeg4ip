SUBDIRS = x86_asm

noinst_LTLIBRARIES = libxvidquant.la
libxvidquant_la_LDFLAGS = -static

AM_CFLAGS= -O3

INCLUDES=-I$(top_builddir)/include
libxvidquant_la_SOURCES = \
	adapt_quant.c \
	adapt_quant.h \
	quant_h263.c \
	quant_h263.h \
	quant_matrix.c \
	quant_matrix.h \
	quant_mpeg4.c \
	quant_mpeg4.h

if USE_MMX
libxvidquant_la_LIBADD = x86_asm/libxvidquantx86.la
endif
