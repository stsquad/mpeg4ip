libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = ffmpeg_video_plugin.la
ffmpeg_video_plugin_la_LDFLAGS = -module 
ffmpeg_video_plugin_la_SOURCES = \
	ffmpeg.cpp ffmpeg.h
ffmpeg_video_plugin_la_LIBADD = \
	-lm \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	@FFMPEG_LIB@

INCLUDES = -I$(top_srcdir)/player/src -I$(top_srcdir)/player/lib -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4av -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp @FFMPEG_INC@

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = 
