libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = rfc3267_plugin.la

rfc3267_plugin_la_LDFLAGS = -module

rfc3267_plugin_la_SOURCES = \
	rfc3267.cpp rfc3267.h

rfc3267_plugin_la_LIBADD = \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	-lm 

INCLUDES = -I$(top_srcdir)/player/src \
	   -I$(top_srcdir)/player/lib \
	   -I$(top_srcdir)/include \
           -I$(top_srcdir)/lib/sdp \
           -I$(top_srcdir)/lib/rtp \
           -I$(top_srcdir)/lib/mp4v2 \
           -I$(top_srcdir)/lib/gnu \
           -I$(top_srcdir)/lib/mp4av

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = 
