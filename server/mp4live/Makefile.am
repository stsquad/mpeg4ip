SUBDIRS = gui

bin_PROGRAMS = mp4live

mp4live_SOURCES = \
	mp4live.cpp \
	mp4live.h \
	config_set.h \
	mp4live_config.h \
	mp4live_config.cpp \
	media_codec.h \
	media_flow.h \
	media_flow.cpp \
	media_frame.h \
	media_node.h \
	media_sink.h \
	media_source.h \
	media_source.cpp \
	media_time.h \
	audio_encoder.h \
	audio_encoder.cpp \
	audio_faac.h \
	audio_faac.cpp \
	audio_lame.h \
	audio_lame.cpp \
	audio_oss_source.cpp \
	audio_oss_source.h \
	video_encoder.h \
	video_encoder.cpp \
	video_h26l.h \
	video_h26l.cpp \
	video_xvid.h \
	video_xvid.cpp \
	video_v4l_source.h \
	video_v4l_source.cpp \
	video_sdl_preview.h \
	video_sdl_preview.cpp \
	video_util_mpeg4.cpp \
	video_util_resize.h \
	video_util_resize.cpp \
	video_util_rgb.h \
	video_util_rgb.cpp \
	video_util_tv.h \
	video_util_tv.cpp \
	file_mp4_recorder.h \
	file_mp4_recorder.cpp \
	file_raw_sink.h \
	file_raw_sink.cpp \
	rtp_transmitter.cpp \
	rtp_transmitter.h \
	sdp_file.cpp \
	util.h \
	util.cpp

# LATER
# video_1394_source
# video_dv

INCLUDES=\
	@GLIB_CFLAGS@ @GTK_CFLAGS@ \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/SDL/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/msg_queue \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/sdp \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/server/audio/faac \
	-I$(top_srcdir)/lib/xvid \
	-I$(top_srcdir)/server/video/H26L \
	-I$(top_srcdir)/common/video/libmpeg32 \
	-I$(top_srcdir)/player/lib \
	-I$(top_srcdir)/player/src

AM_CXXFLAGS = -DADD_XVID_ENCODER \
	-DADD_LAME_ENCODER -DADD_FAAC_ENCODER \
	-D_REENTRANT -DNOCONTROLS -fexceptions -Wall

LDADD = \
	@GTK_LIBS@ @GLIB_LIBS@ \
	$(top_builddir)/server/mp4live/gui/libmp4livegui.la \
	$(top_builddir)/server/audio/faac/libfaac.la \
	-lmp3lame \
	$(top_builddir)/lib/xvid/libxvid.la \
	$(top_builddir)/server/video/H26L/libh26lenc.la \
	$(top_builddir)/common/video/libmpeg32/libmpeg3.la \
	$(top_builddir)/lib/msg_queue/libmsg_queue.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/rtp/libuclmmbase.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	-lpthread \
	@SDL_LIBS@ \
	$(top_builddir)/lib/SDL/src/main/libSDLmain.a 

EXTRA_DIST = INTERNALS
