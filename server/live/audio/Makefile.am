noinst_LTLIBRARIES = liblive_audio.la
liblive_audio_la_LDFLAGS = -static

liblive_audio_la_SOURCES = \
	audio.cpp \
	audio_apis.cpp \
	mp3.cpp \
	mp3_file.cpp

INCLUDES=-I.. -I$(top_builddir)/player/lib/SDL/include -I$(top_builddir)/common/lib/msg_queue -I$(top_builddir)/server/audio/lame

AM_CFLAGS= -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -fno-rtti -Wall -Werror
