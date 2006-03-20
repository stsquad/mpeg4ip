noinst_LTLIBRARIES = libmp4livegui.la

libmp4livegui_la_SOURCES = \
	audio_dialog.cpp \
	encoder_dialog.cpp \
	file_utils.cpp \
	file_utils.h \
	gui_main.cpp \
	gui_private.h \
	gui_utils.cpp \
	gui_utils.h \
	mp4live_gui.h \
	picture_dialog.cpp \
	preferences.cpp \
	support.cpp \
	support.h \
	text_dialog.cpp \
	transmit_dialog.cpp \
	video_dialog.cpp 

INCLUDES=@GLIB_CFLAGS@ @GTK_CFLAGS@ \
	-I$(top_srcdir)/server/mp4live \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/msg_queue \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/srtp \
	-I$(top_srcdir)/lib/sdp \
	-I$(top_srcdir)/lib/utils \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/mpeg2ps \
	-I$(top_srcdir)/player/lib

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

