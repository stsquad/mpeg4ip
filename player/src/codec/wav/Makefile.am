lib_LTLIBRARIES = wav_plugin.la
wav_plugin_la_LDFLAGS = -module
wav_plugin_la_SOURCES = \
	ourwav.cpp \
	ourwav.h \
	wav_file.cpp \
	wav_file.h

libdir = @PLAYER_PLUGIN_DIR@

INCLUDES = -I.. -I$(top_srcdir)/player/src  -I$(top_srcdir)/player/lib -I$(top_srcdir)/player/lib/audio -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@
