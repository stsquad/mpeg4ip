noinst_LTLIBRARIES = libmp4livegui.la

libmp4livegui_la_SOURCES = \
	mp4live_gui.h \
	gui_main.cpp \
	gui_private.h \
	audio_dialog.cpp \
	picture_dialog.cpp \
	recording_dialog.cpp \
	transmit_dialog.cpp \
	video_dialog.cpp \
	gui_utils.h \
	gui_utils.cpp \
	support.cpp \
	support.h

INCLUDES=@GLIB_CFLAGS@ @GTK_CFLAGS@ \
	-I.. \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/msg_queue \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/sdp \
	-I$(top_srcdir)/lib/utils \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/mpeg2ps \
	-I$(top_srcdir)/player/lib

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

