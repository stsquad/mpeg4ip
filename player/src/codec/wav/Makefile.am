noinst_LTLIBRARIES = libourwav.la
libourwav_la_LDFLAGS = -static
libourwav_la_SOURCES = \
	ourwav.cpp \
	wav_file.cpp

INCLUDES = -I.. -I../..  -I../../../lib -I../../../lib/audio -I../../../lib/SDL/include -I../../../../common/video/mpeg4 -I../../../../include

AM_CFLAGS = -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -fno-rtti -Wall -Werror -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes
