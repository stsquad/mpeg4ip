libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = celp_plugin.la
celp_plugin_la_LDFLAGS = -module
celp_plugin_la_SOURCES = \
	celp.cpp celp.h
celp_plugin_la_LIBADD = \
	-lm \
	$(top_srcdir)/player/lib/audio/celp/BS/libcelpbs.la \
	$(top_srcdir)/player/lib/audio/celp/dec/libcelp.la 

INCLUDES = -I$(top_srcdir)/player/src -I$(top_srcdir)/player/lib -I$(top_srcdir)/lib/SDL/include -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/player/lib/audio/celp -I$(top_srcdir)/player/lib/audio/celp/BS -I$(top_srcdir)/player/lib/audio/celp/dec -I$(top_srcdir)/player/lib/audio/celp/include -I$(top_srcdir)/lib/mp4av

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = celp_file.cpp celp_plugin.dsp
