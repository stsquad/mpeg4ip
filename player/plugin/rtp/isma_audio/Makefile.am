libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = isma_rtp_plugin.la
isma_rtp_plugin_la_LDFLAGS = -module
isma_rtp_plugin_la_SOURCES = \
	isma_rtp_plugin.cpp isma_rtp_plugin.h
isma_rtp_plugin_la_LIBADD = \
	$(top_srcdir)/lib/mp4av/libmp4av.la \
	$(top_srcdir)/player/lib/mp4util/libmp4util.la \
	-lm 

INCLUDES = -I$(top_srcdir)/player/src \
	   -I$(top_srcdir)/player/lib \
	   -I$(top_srcdir)/include \
           -I$(top_srcdir)/lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = isma_audio_rtp_plugin.dsp
