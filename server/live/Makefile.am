SUBDIRS = audio gui video

bin_PROGRAMS = mp4live

mp4live_SOURCES = \
	gui_main.cpp \
	record_apis.cpp \
	util.c

INCLUDES=-I/usr/lib/glib/include -I./gui -I./audio -I./video -I$(top_builddir)/include

AM_CFLAGS= -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -fno-rtti -Wall -Werror

LDADD = \
	gui/liblive_gui.la \
	audio/liblive_audio.la \
	video/liblive_video.la \
	$(top_builddir)/common/lib/msg_queue/libmsg_queue.la \
	$(top_builddir)/server/audio/lame/libmp3lame.a \
	-lpthread \
	$(top_builddir)/player/lib/SDL/src/libSDL.la \
	-L/usr/lib -L/usr/X11R6/lib -lgtk -lgdk \
	-lgmodule -lglib -lXi -lXext -lX11 -lm
