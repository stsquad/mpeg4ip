SUBDIRS = gui

bin_PROGRAMS = mp4live

mp4live_SOURCES = \
	audio_source.cpp \
	audio_source.h \
	config_set.h \
	live_config.h \
	mp4live.cpp \
	mp4live.h \
	media_flow.h \
	media_flow.cpp \
	media_frame.h \
	media_node.h \
	mp3.h \
	mp3.cpp \
	mp4_recorder.cpp \
	mp4_recorder.h \
	mpeg4vhdr.cpp \
	raw_recorder.cpp \
	raw_recorder.h \
	rgb2yuv.h \
	rgb2yuv.cpp \
	rtp_transmitter.cpp \
	rtp_transmitter.h \
	sdp_file.cpp \
	timestamp.h \
	tv_frequencies.cpp \
	tv_frequencies.h \
	util.c \
	util.h \
	video_source.cpp \
	video_source.h

INCLUDES=\
	@GLIB_CFLAGS@ @GTK_CFLAGS@ \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/SDL/include \
	-I$(top_srcdir)/lib/mp4 \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/msg_queue \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/sdp \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/server/audio/lame \
	-I$(top_srcdir)/server/video/divx \
	-I$(top_srcdir)/server/video/ffmpeg \
	-I$(top_srcdir)/player/lib

AM_CFLAGS= -DMPEG4IP -D_REENTRANT -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes 

AM_CXXFLAGS = -DMPEG4IP -D_REENTRANT -DNOCONTROLS -fexceptions -Wall

LDADD = \
	$(top_builddir)/server/mp4live/gui/liblive_gui.la \
	$(top_builddir)/lib/msg_queue/libmsg_queue.la \
	$(top_builddir)/lib/mp4/libmp4.la \
	$(top_builddir)/lib/rtp/libuclmmbase.a \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/server/audio/lame/libmp3lame.a \
	$(top_builddir)/server/video/divx/libdivxencore.la \
	$(top_builddir)/server/video/ffmpeg/libffmpeg.la \
	-lpthread \
	$(top_builddir)/lib/SDL/src/libSDL.la \
	@GTK_LIBS@ @GLIB_LIBS@
