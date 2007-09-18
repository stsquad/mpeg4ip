lib_LTLIBRARIES = libmp4util.la 
noinst_LTLIBRARIES =  libh264util.la
include_HEADERS = \
	mpeg4_audio_config.h \
	mpeg4_sdp.h \
	h264_sdp.h

libmp4util_la_SOURCES = \
	mpeg4_audio_config.cpp \
        mpeg4_sdp.c

libh264util_la_SOURCES = \
	h264_sdp.c

libh264util_la_LIBADD = \
	$(top_builddir)/lib/mp4v2/libmp4v2.la

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4v2

AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = libmp4util60.dsp libmp4util.vcproj
