libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = rfc2429_rtp_plugin.la
rfc2429_rtp_plugin_la_LDFLAGS = -module
rfc2429_rtp_plugin_la_SOURCES = \
	rfc2429_plugin.cpp rfc2429_plugin.h 

rfc2429_rtp_plugin_la_LIBADD = \
	-lm 

INCLUDES = -I$(top_srcdir)/../mpeg4ip/player/src \
	   -I$(top_srcdir)/../mpeg4ip/player/lib \
	   -I$(top_srcdir)/../mpeg4ip/lib/SDL/include \
	   -I$(top_srcdir)/../mpeg4ip/include \
           -I$(top_srcdir)/../mpeg4ip/lib \
           -I$(top_srcdir)/../mpeg4ip/lib/rtp \
           -I$(top_srcdir)/../mpeg4ip/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

