libdir = @PLAYER_PLUGIN_DIR@

if HAVE_MAC_OSX
AM_LDFLAGS = -Wl,-read_only_relocs -Wl,suppress 
else
AM_LDFLAGS = 
endif

lib_LTLIBRARIES = ffmpeg_video_plugin.la
ffmpeg_video_plugin_la_LDFLAGS = -module 
ffmpeg_video_plugin_la_SOURCES = \
	ffmpeg.cpp ffmpeg.h
ffmpeg_video_plugin_la_LIBADD = \
	-lm \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	$(top_builddir)/lib/utils/libmutex.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/player/lib/mp4util/libh264util.la \
	$(top_builddir)/lib/ffmpeg/libmpeg4ip_ffmpeg.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	@FFMPEG_LIB@

INCLUDES = \
	-I$(top_srcdir)/player/src \
	-I$(top_srcdir)/player/lib \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/ffmpeg \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/sdp \
	@FFMPEG_INC@

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = 
