
noinst_LTLIBRARIES = libourmp3.la
libourmp3_la_LDFLAGS = -static
libourmp3_la_SOURCES =		\
	MPEGaudio.cpp		\
	bitwindow.cpp		\
	filter.cpp		\
	filter_2.cpp		\
	huffmantable.cpp	\
	mpeglayer1.cpp		\
	mpeglayer2.cpp		\
	mpeglayer3.cpp		\
	mpegtable.cpp		\
	mpegtoraw.cpp           \
	ourmpegaudio.cpp \
	MPEGaction.h \
	MPEGaudio.h \
	MPEGerror.h \
	MPEGfilter.h \
	MPEGring.h

EXTRA_DIST =			\
	AUTHORS			\
	COPYING.LIB		\
	README			\
	README.LIB 		\
	libmp360.dsp 

AM_CXXFLAGS = -D_REENTRANT -Werror -Wall -fexceptions -I$(top_srcdir)/lib/SDL/include -I../../../../include
