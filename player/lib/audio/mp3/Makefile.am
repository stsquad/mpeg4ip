
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
	ourmpegaudio.cpp

EXTRA_DIST =			\
	AUTHORS			\
	COPYING.LIB		\
	README			\
	README.LIB

AM_CXXFLAGS = -D_REENTRANT -Werror -Wall -fexceptions -fno-rtti -I../../SDL/include -I../../../../include
