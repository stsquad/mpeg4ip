libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = g711_audio_plugin.la
g711_audio_plugin_la_LDFLAGS = -module
g711_audio_plugin_la_SOURCES = \
	ourg711.h ourg711.cpp
g711_audio_plugin_la_LIBADD = \
	$(top_srcdir)/lib/mp4av/libmp4av.la \
	$(top_srcdir)/lib/mp4v2/libmp4v2.la \
	-lm  

INCLUDES = -I$(top_srcdir)/player/src -I$(top_srcdir)/include -I$(top_srcdir)/lib 

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

