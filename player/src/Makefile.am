SUBDIRS = codec

bin_PROGRAMS = mp4player gmp4player

check_PROGRAMS = movplayer

mp4player_SOURCES = \
	audio.cpp \
	ip_port.cpp \
	main.cpp \
	media_utils.cpp \
	msg_queue.cpp \
	our_bytestream_file.cpp \
	player_media.cpp \
	player_media_decode.cpp \
	player_mem_bytestream.cpp \
	player_rtp_bytestream.cpp \
	player_sdp.c \
	player_session.cpp \
	player_util.c \
	qtime_bytestream.cpp \
	qtime_file.cpp \
	sync.cpp \
	video.cpp

gmp4player_SOURCES = \
	audio.cpp \
	gui_main.cpp \
	gui_showmsg.c \
	gui_utils.c \
	ip_port.cpp \
	media_utils.cpp \
	msg_queue.cpp \
	our_bytestream_file.cpp \
	player_media.cpp \
	player_media_decode.cpp \
	player_mem_bytestream.cpp \
	player_rtp_bytestream.cpp \
	player_sdp.c \
	player_session.cpp \
	player_util.c \
	qtime_bytestream.cpp \
	qtime_file.cpp \
	sync.cpp \
	video.cpp

movplayer_SOURCES = player_util.c \
	movplayer.cpp

INCLUDES = -I. -I$(top_srcdir)/common/video/mpeg4 -I./codec -I./codec/mpeg4 -I../lib -I../lib/audio -I../lib/SDL/include -I$(top_srcdir)/common/mp4 -I$(top_srcdir)/include -I/usr/lib/glib/include -I$(top_srcdir)/include

AM_CFLAGS= -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -fno-rtti -Wall -Werror

LDADD = \
	$(top_builddir)/player/lib/sdp/libsdp.la \
	$(top_builddir)/player/lib/rtsp/librtsp.la \
	$(top_builddir)/player/lib/rtp/libuclmmbase.a \
	$(top_builddir)/player/lib/audio/faad/libfaad.la \
	$(top_builddir)/player/src/codec/aa/libaa.la \
	$(top_builddir)/player/lib/audio/faad/libfaad.la \
	$(top_builddir)/player/src/codec/mpeg4/libmpeg4.la \
	$(top_builddir)/player/lib/video/divx/libourdivx.la \
	$(top_builddir)/player/src/codec/divx/libdivx.la \
	$(top_builddir)/player/lib/video/divx/libourdivx.la \
	$(top_builddir)/common/video/mpeg4/obj/libobj.a \
	$(top_builddir)/player/lib/SDL/src/libSDL.la \
	$(top_builddir)/common/mp4/libmp4.la \
	-lpthread \
	-lm \
	$(top_builddir)/player/lib/video/divx/libourdivx.la \
	$(top_builddir)/player/src/codec/aa/libaa.la \
	$(top_builddir)/common/video/mpeg4/obj/libobj.a \
	-L/usr/lib -L/usr/X11R6/lib -lgtk -lgdk \
	 -lgmodule -lglib -ldl -lXi -lXext -lX11 -lm

LDFLAGS = -rdynamic
