bin_PROGRAMS = faac

faac_SOURCES = faac.c

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/server/lib/sndfile/src

AM_CFLAGS = -O2

LDADD = $(top_builddir)/server/lib/sndfile/src/libsndfile.la \
	libfaac.la -lm

lib_LTLIBRARIES = libfaac.la

libfaac_la_SOURCES = \
	aacquant.h \
	aacquant.c \
	backpred.h \
	backpred.c \
	bitstream.h \
	bitstream.c \
	channels.h \
	channels.c \
	coder.h \
	fft.h \
	fft.c \
	filtbank.h \
	filtbank.c \
	frame.h \
	frame.c \
	huffman.h \
	hufftab.h \
	huffman.c \
	joint.h \
	joint.c \
	ltp.h \
	ltp.c \
	psych.h \
	psych.c \
	tns.h \
	tns.c \
	util.h \
	util.c

