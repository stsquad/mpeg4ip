
INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/avi \
	-I$(top_srcdir)/lib/bitstream

AM_CXXFLAGS = -DMPEG4IP @BILLS_CPPWARNINGS@

lib_LTLIBRARIES = libmp4av.la libh264av.la

include_HEADERS = \
	mp4av_aac.h \
	mp4av_adts.h \
	mp4av_audio.h \
	mp4av.h \
	mp4av_h264.h \
	mp4av_hinters.h \
	mp4av_mp3.h \
	mp4av_mpeg3.h \
	mp4av_mpeg4.h 

libmp4av_la_SOURCES = \
	aac.cpp \
	adts.cpp \
	audio.cpp \
	audio_hinters.cpp \
	audio_hinters.h \
	l16.cpp \
	mp3.cpp \
	mbs.h \
	mbs.cpp \
	mp4av_common.h \
	mpeg3.cpp \
	mpeg4.cpp \
	rfc2250.cpp \
	rfc3016.cpp \
	rfc3119.cpp \
        rfccrypto.cpp \
	rfcisma.cpp

libh264av_la_SOURCES = \
	h264.cpp 

EXTRA_DIST = libmp4av.dsp libmp4av_st.dsp libmp4av.xcode/project.pbxproj

