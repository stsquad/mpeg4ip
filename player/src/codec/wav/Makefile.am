noinst_LTLIBRARIES = libourwav.la
libourwav_la_LDFLAGS = -static
libourwav_la_SOURCES = \
	ourwav.cpp \
	wav_file.cpp

INCLUDES = -I.. -I../..  -I../../../lib -I../../../lib/audio -I$(top_srcdir)/lib/SDL/include -I../../../../common/video/mpeg4 -I../../../../include -I../../../../lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@
