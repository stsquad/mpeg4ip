libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = mpeg3_video_plugin.la
mpeg3_video_plugin_la_LDFLAGS = -module
mpeg3_video_plugin_la_SOURCES = \
	mpeg3.cpp mpeg3.h
mpeg3_video_plugin_la_LIBADD = \
	-lm \
	$(top_builddir)/common/video/libmpeg32/video/libmpeg3video.la \
	$(top_builddir)/common/video/libmpeg32/libmpeg3util.la \
	@SDL_LIB_LIBS@

INCLUDES = -I$(top_srcdir)/player/src -I$(top_srcdir)/player/lib -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/common/video/libmpeg32 -I$(top_srcdir)/common/video/libmpeg32/video -I$(top_srcdir)/lib/mp4av -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = mpeg3_plugin.dsp
