SUBDIRS = gui

bin_PROGRAMS = mp4live

mp4live_SOURCES = \
	mp4live.cpp \
	mp4live.h \
	media_frame.h \
	media_node.h \
	audio_source.cpp \
	audio_source.h \
	mp4_recorder.cpp \
	mp4_recorder.h \
	raw_recorder.cpp \
	raw_recorder.h \
	rtp_transmitter.cpp \
	rtp_transmitter.h \
	video_source.cpp \
	video_source.h \
	timestamp.h \
	tv_frequencies.c \
	tv_frequencies.h \
	util.c \
	util.h 

INCLUDES=\
	@GLIB_INCLUDES@ @GTK_INCLUDES@ \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/player/lib/SDL/include \
	-I$(top_srcdir)/common/mp4 \
	-I$(top_srcdir)/common/lib/msg_queue \
	-I$(top_srcdir)/server/audio/lame \
	-I$(top_srcdir)/server/video/divx \
	-I$(top_srcdir)/server/video/ffmpeg \
	-I$(top_srcdir)/common/mp4 \
	-I$(top_srcdir)/player/lib/rtp \
	-I$(top_srcdir)/player/lib/sdp \
	-I$(top_srcdir)/player/lib

AM_CFLAGS= -DMPEG4IP -D_REENTRANT -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes 

AM_CXXFLAGS = -DMPEG4IP -D_REENTRANT -DNOCONTROLS -fexceptions -Wall

LDADD = \
	$(top_builddir)/server/live/gui/liblive_gui.la \
	$(top_builddir)/common/lib/msg_queue/libmsg_queue.la \
	$(top_builddir)/server/audio/lame/libmp3lame.a \
	$(top_builddir)/server/video/divx/libdivxencore.la \
	$(top_builddir)/server/video/ffmpeg/libffmpeg.la \
	$(top_builddir)/common/mp4/libmp4.la \
	$(top_builddir)/player/lib/rtp/libuclmmbase.a \
	$(top_builddir)/player/lib/sdp/libsdp.la \
	-lpthread \
	$(top_builddir)/player/lib/SDL/src/libSDL.la \
	-L/usr/lib -L/usr/X11R6/lib -lgtk -lgdk \
	-lgmodule -lglib -lXi -lXext -lX11 -lm
