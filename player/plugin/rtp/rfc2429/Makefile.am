libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = rfc2429_rtp_plugin.la
rfc2429_rtp_plugin_la_LDFLAGS = -module
rfc2429_rtp_plugin_la_SOURCES = \
	rfc2429_plugin.cpp rfc2429_plugin.h 

rfc2429_rtp_plugin_la_LIBADD = \
	-lm 

INCLUDES = -I$(top_srcdir)/player/src \
	   -I$(top_srcdir)/player/lib \
	   -I$(top_srcdir)/lib/SDL/include \
	   -I$(top_srcdir)/include \
           -I$(top_srcdir)/lib \
           -I$(top_srcdir)/lib/rtp \
           -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

