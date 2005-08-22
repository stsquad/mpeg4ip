libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = h264_rtp_plugin.la 

h264_rtp_plugin_la_LDFLAGS = -module

h264_rtp_plugin_la_SOURCES = \
	h264_rtp_plugin.cpp h264_rtp_plugin.h

h264_rtp_plugin_la_LIBADD = \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	-lm 

INCLUDES = -I$(top_srcdir)/player/src \
	   -I$(top_srcdir)/player/lib \
	   -I$(top_srcdir)/include \
           -I$(top_srcdir)/lib \
           -I$(top_srcdir)/lib/rtp \
           -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

