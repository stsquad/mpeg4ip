lib_LTLIBRARIES = wav_plugin.la
wav_plugin_la_LDFLAGS = -module
wav_plugin_la_SOURCES = \
	ourwav.cpp \
	ourwav.h \
	wav_file.cpp \
	wav_file.h

libdir = @PLAYER_PLUGIN_DIR@

INCLUDES = -I.. -I../..  -I../../../lib -I../../../lib/audio -I$(top_srcdir)/lib/SDL/include -I../../../../common/video/mpeg4 -I../../../../include -I../../../../lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@
