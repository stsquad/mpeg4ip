noinst_LTLIBRARIES = liblive_gui.la
liblive_gui_la_LDFLAGS = -static

liblive_gui_la_SOURCES = \
	audio_dialog.cpp \
	broadcast_dialog.cpp \
	gui_showmsg.c \
	gui_utils.c \
	recording_dialog.cpp \
	video_dialog.cpp

INCLUDES=-I/usr/lib/glib/include -I..

AM_CFLAGS= -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -fno-rtti -Wall -Werror
