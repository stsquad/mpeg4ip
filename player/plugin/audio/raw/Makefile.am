libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = raw_audio_plugin.la
raw_audio_plugin_la_LDFLAGS = -module
raw_audio_plugin_la_SOURCES = \
	rawa.cpp rawa.h
raw_audio_plugin_la_LIBADD = \
	-lm 

INCLUDES = -I$(top_srcdir)/player/src -I$(top_src_dir)/player/lib -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

