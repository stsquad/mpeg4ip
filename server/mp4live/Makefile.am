SUBDIRS = gui

bin_PROGRAMS = mp4live

mp4live_SOURCES = \
	audio_encoder.h \
	audio_faac.h \
	audio_faac.cpp \
	audio_lame.h \
	audio_lame.cpp \
	audio_source.cpp \
	audio_source.h \
	config_set.h \
	mp4live.cpp \
	mp4live.h \
	mp4live_config.h \
	mp4live_config.cpp \
	media_codec.h \
	media_flow.h \
	media_flow.cpp \
	media_frame.h \
	media_node.h \
	mp3.h \
	mp3.cpp \
	mp4_recorder.cpp \
	mp4_recorder.h \
	mpeg4vhdr.cpp \
	rgb2yuv.h \
	rgb2yuv.cpp \
	rtp_transmitter.cpp \
	rtp_transmitter.h \
	sdp_file.cpp \
	timestamp.h \
	transcoder.h \
	transcoder.cpp \
	tv_frequencies.cpp \
	tv_frequencies.h \
	util.h \
	util.cpp \
	video_encoder.h \
	video_ffmpeg.h \
	video_ffmpeg.cpp \
	video_preview.h \
	video_preview.cpp \
	video_source.cpp \
	video_source.h \
	video_xvid.h \
	video_xvid.cpp

# Note that Divx and Xvid libraries have name conflict in their putvlc.c
# code so don't attempt to link both at once. 

INCLUDES=\
	@GLIB_CFLAGS@ @GTK_CFLAGS@ \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/SDL/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/msg_queue \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/sdp \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/server/audio/faac \
	-I$(top_srcdir)/server/audio/lame \
	-I$(top_srcdir)/server/video/ffmpeg \
	-I$(top_srcdir)/common/video/xvid \
	-I$(top_srcdir)/player/lib

AM_CXXFLAGS = -DADD_FFMPEG_ENCODER -DADD_XVID_ENCODER \
	-DADD_LAME_ENCODER -DADD_FAAC_ENCODER \
	-DTRANSCODER \
	-D_REENTRANT -DNOCONTROLS -fexceptions -Wall

LDADD = \
	$(top_builddir)/server/mp4live/gui/liblive_gui.la \
	$(top_builddir)/lib/msg_queue/libmsg_queue.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/rtp/libuclmmbase.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/server/audio/faac/libfaac.la \
	$(top_builddir)/server/audio/lame/libmp3lame.a \
	$(top_builddir)/server/video/ffmpeg/libffmpeg.la \
	$(top_builddir)/common/video/xvid/libxvid.la \
	-lpthread \
	$(top_builddir)/lib/SDL/src/libSDL.la \
	@GTK_LIBS@ @GLIB_LIBS@
