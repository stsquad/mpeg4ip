
INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/avi \
	-I$(top_srcdir)/lib/bitstream

AM_CXXFLAGS = -DMPEG4IP @BILLS_CPPWARNINGS@

lib_LTLIBRARIES = libmp4av.la

include_HEADERS = \
	mp4av_aac.h \
	mp4av_ac3.h \
	mp4av_adts.h \
	mp4av_amr.h \
	mp4av_audio.h \
	mp4av.h \
	mp4av_h264.h \
	mp4av_hinters.h \
	mp4av_mp3.h \
	mp4av_mpeg3.h \
	mp4av_mpeg4.h 

libmp4av_la_SOURCES = \
	aac.cpp \
	ac3.cpp \
	adts.cpp \
	amr.cpp \
	audio.cpp \
	audio_hinters.cpp \
	audio_hinters.h \
	h264.cpp \
	href.cpp \
	l16.cpp \
	mp3.cpp \
	mbs.h \
	mbs.cpp \
	mp4av_common.h \
	mpeg3.cpp \
	mpeg4.cpp \
	rfc2250.cpp \
	rfc2429.cpp \
	rfc3016.cpp \
	rfc3119.cpp \
	rfc3267.cpp \
        rfccrypto.cpp \
	rfcisma.cpp \
	rfch264.cpp

libmp4av_la_LIBADD = $(top_builddir)/lib/bitstream/libbitstream.la

EXTRA_DIST = libmp4av.dsp libmp4av_st.dsp 

