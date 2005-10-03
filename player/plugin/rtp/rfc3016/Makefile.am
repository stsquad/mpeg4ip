libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = latm_rtp_plugin.la 

latm_rtp_plugin_la_LDFLAGS = -module
latm_rtp_plugin_la_SOURCES = \
	latm.cpp latm.h

latm_rtp_plugin_la_LIBADD = \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	-lm 

INCLUDES = -I$(top_srcdir)/player/src \
	   -I$(top_srcdir)/player/lib \
	   -I$(top_srcdir)/include \
           -I$(top_srcdir)/lib/ismacryp \
           -I$(top_srcdir)/lib \
           -I$(top_srcdir)/lib/sdp \
           -I$(top_srcdir)/lib/rtp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = 
