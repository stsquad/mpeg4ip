noinst_LTLIBRARIES = liblive_video.la
liblive_video_la_LDFLAGS = -static

liblive_video_la_SOURCES = \
	video_apis.cpp

INCLUDES=-I.. -I$(top_builddir)/player/lib/SDL/include -I$(top_builddir)/common/lib/msg_queue

AM_CFLAGS= -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -fno-rtti -Wall -Werror
