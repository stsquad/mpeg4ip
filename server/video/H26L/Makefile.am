bin_PROGRAMS = h26lenc
h26lenc_SOURCES = lencod.c

INCLUDES = -I$(top_srcdir)/include

LDADD = $(top_builddir)/server/video/H26L/libh26lenc.la -lm \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la


AM_CFLAGS = -DMPEG4IP -D_REENTRANT -O3 -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

lib_LTLIBRARIES = libh26lenc.la

libh26lenc_la_SOURCES = \
	b_frame.c \
	b_frame.h \
	biariencode.c \
	biariencode.h \
	block.c \
	block.h \
	cabac.c \
	cabac.h \
	configfile.c \
	configfile.h \
	contributors.h \
	decoder.c \
	defines.h \
	elements.h \
	filehandle.c \
	global.h \
	header.c \
	header.h \
	image.c \
	image.h \
	leaky_bucket.c \
	leaky_bucket.h \
	loopFilter.c \
	macroblock.c \
	macroblock.h \
	minmax.h \
	mv-search.c \
	mv-search.h \
	rdopt.c \
	rdopt_coding_state.c \
	rdopt_coding_state.h \
	rdopt.h \
	refbuf.c \
	refbuf.h \
	rtp.c \
	rtp.h \
	uvlc.c 
