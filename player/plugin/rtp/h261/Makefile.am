libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = h261_rtp_plugin.la
h261_rtp_plugin_la_LDFLAGS = -module
h261_rtp_plugin_la_SOURCES = \
	h261_rtp_plugin.cpp h261_rtp_plugin.h \
	h261_userdata.h

h261_rtp_plugin_la_LIBADD = \
	-lm 

INCLUDES = -I$(top_srcdir)/player/src \
	   -I$(top_srcdir)/player/lib \
	   -I$(top_srcdir)/include \
           -I$(top_srcdir)/lib \
           -I$(top_srcdir)/lib/sdp \
           -I$(top_srcdir)/lib/rtp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

