lib_LTLIBRARIES = libmp4util.la
include_HEADERS = \
	mpeg4_audio_config.h \
	mpeg4_sdp.h 

libmp4util_la_SOURCES = \
	mpeg4_audio_config.cpp \
        mpeg4_sdp.c

libmp4util_la_LIBADD = \
        $(top_srcdir)/lib/bitstream/libbitstream.la

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib

AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = libmp4util60.dsp
