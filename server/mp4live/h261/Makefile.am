noinst_LTLIBRARIES = libmp4live_h261.la

libmp4live_h261_la_SOURCES = encoder-h261.cpp \
	bsd-endian.h \
	crdef.cpp \
	crdef.h \
	dct.cpp \
	dct.h \
	encoder-h261.h \
	huffcode.c \
	p64-huff.h

INCLUDES = -I$(top_srcdir)/server/mp4live \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/ \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/msg_queue \
	-I$(top_srcdir)/lib/utils \
	-I$(top_srcdir)/lib/srtp \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/sdp 

AM_CXXFLAGS = \
	-D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

AM_CFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CWARNINGS@

