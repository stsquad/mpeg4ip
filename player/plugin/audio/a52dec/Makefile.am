libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = a52_audio_plugin.la
a52_audio_plugin_la_LDFLAGS = -module
a52_audio_plugin_la_SOURCES = \
	a52dec.cpp a52dec.h
a52_audio_plugin_la_LIBADD = \
	-lm -la52

INCLUDES = -I$(top_srcdir)/player/src -I$(top_src_dir)/player/lib -I$(top_srcdir)/lib/SDL/include -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

