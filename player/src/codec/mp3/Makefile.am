noinst_LTLIBRARIES = libmp3.la
libmp3_la_LDFLAGS = -static
libmp3_la_SOURCES = \
	mp3.cpp \
	mp3_file.cpp

INCLUDES = -I.. -I../..  -I../../../lib -I../../../lib/audio -I$(top_srcdir)/lib/SDL/include -I../../../../common/video/mpeg4 -I../../../../include -I../../../../lib

AM_CFLAGS = -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes
