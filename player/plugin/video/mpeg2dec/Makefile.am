libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = mpeg2dec_video_plugin.la
mpeg2dec_video_plugin_la_LDFLAGS = -module
mpeg2dec_video_plugin_la_SOURCES = \
	mpeg2dec.cpp mpeg2dec.h
mpeg2dec_video_plugin_la_LIBADD = \
	-lm \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	-lmpeg2

INCLUDES = -I$(top_srcdir)/player/src -I$(top_srcdir)/player/lib -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4av -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = 
