noinst_LTLIBRARIES = liblive_gui.la
liblive_gui_la_LDFLAGS = -static

liblive_gui_la_SOURCES = \
	mp4live_gui.h \
	gui_main.cpp \
	gui_private.h \
	gui_showmsg.cpp \
	gui_utils.cpp \
	gui_utils.h \
	audio_dialog.cpp \
	recording_dialog.cpp \
	transmit_dialog.cpp \
	video_dialog.cpp

INCLUDES=@GLIB_INCLUDES@ @GTK_INCLUDES@ \
	-I.. \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/SDL/include \
	-I$(top_srcdir)/lib/mp4 \
	-I$(top_srcdir)/lib/msg_queue \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/sdp \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/server/audio/lame \
	-I$(top_srcdir)/server/video/divx \
	-I$(top_srcdir)/server/video/ffmpeg \
	-I$(top_srcdir)/player/lib

AM_CFLAGS= -D_REENTRANT -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall
