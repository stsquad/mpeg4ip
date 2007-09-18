libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = href_rtp_plugin.la
href_rtp_plugin_la_LDFLAGS = -module
href_rtp_plugin_la_SOURCES = \
	href_plugin.cpp href_plugin.h 

href_rtp_plugin_la_LIBADD = \
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

EXTRA_DIST = isma_href_rtp_plugin.dsp isma_href_rtp_plugin.vcproj

