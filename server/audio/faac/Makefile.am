bin_PROGRAMS = faac

faac_SOURCES = faac.c

INCLUDES = -I$(top_srcdir)/include 

AM_CFLAGS = -O2

if HAVE_LIBSNDFILE
LD_E1 = -lsndfile
else
LD_E1 = 
endif 

LDADD = $(LD_E1) \
	libfaac.la -lm

noinst_LTLIBRARIES = libfaac.la
libfaac_la_LDFLAGS=-static

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
	faac.h \
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

EXTRA_DIST = COPYING libfaac.pdf faac60.dsp faac60.dsw
