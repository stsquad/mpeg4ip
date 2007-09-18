libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = celp_plugin.la
celp_plugin_la_LDFLAGS = -module
celp_plugin_la_SOURCES = \
	celp.cpp celp.h
celp_plugin_la_LIBADD = \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	$(top_builddir)/player/lib/audio/celp/BS/libmpeg4ip_celpbs.la \
	$(top_builddir)/player/lib/audio/celp/dec/libmpeg4ip_celp.la 

INCLUDES = -I$(top_srcdir)/player/src -I$(top_srcdir)/player/lib -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/player/lib/audio/celp -I$(top_srcdir)/player/lib/audio/celp/BS -I$(top_srcdir)/player/lib/audio/celp/dec -I$(top_srcdir)/player/lib/audio/celp/include -I$(top_srcdir)/lib/mp4av -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/lib/mp4v2

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = celp_file.cpp celp_plugin.dsp celp_plugin.vcproj
