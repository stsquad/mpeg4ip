noinst_LTLIBRARIES = libmp4live_h261.la

libmp4live_h261_la_LDFLAGS = -static
libmp4live_h261_la_SOURCES = encoder-h261.cpp \
	encoder-h261.h \
	crdef.cpp \
	crdef.h \
	dct.cpp \
	dct.h \
	huffcode.c 

INCLUDES = -I.. \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/msg_queue \
	-I$(top_srcdir)/lib/sdp \
	-I$(top_srcdir)/lib/SDL/include 

AM_CXXFLAGS = \
	-D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

AM_CFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CWARNINGS@

