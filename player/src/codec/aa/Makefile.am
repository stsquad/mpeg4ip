noinst_LTLIBRARIES = libaa.la
libaa_la_LDFLAGS = -static
libaa_la_SOURCES = \
	aa.cpp \
	aa_file.cpp \
	aac_rtp_bytestream.cpp \
	isma_rtp_bytestream.cpp

INCLUDES = -I.. -I../..  -I../../../lib -I../../../lib/audio -I$(top_srcdir)/lib/SDL/include -I../../../../common/video/mpeg4 -I../../../../include -I../../../../common/lib -I../../../../lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

