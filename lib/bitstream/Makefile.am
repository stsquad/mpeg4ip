noinst_LTLIBRARIES = libbitstream.la
libbitstream_la_LDFLAGS = -static
libbitstream_la_SOURCES = \
	bitstream.cpp \
	bitstream.h

EXTRA_DIST = bitstream60.dsp 

INCLUDES = -I../../include

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

