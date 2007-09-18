libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = mp3_plugin.la
mp3_plugin_la_LDFLAGS = -module
mp3_plugin_la_SOURCES = \
	mp3if.cpp \
	mp3if.h \
	mp3_file.cpp \
	mp3_file.h \
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
if HAVE_ID3_TAG
   ID3_LIBS=-lid3 -lz
else
   ID3_LIBS=
endif
mp3_plugin_la_LIBADD = \
	-lm $(ID3_LIBS) \
	$(top_builddir)/lib/fposrec/libmpeg4ip_fposrec.la


INCLUDES = -I$(top_srcdir)/player/src -I$(top_srcdir)/player/lib -I$(top_srcdir)/player/lib/audio -I$(top_srcdir)/common/video/mpeg4 -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/lib/utils

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = mp3_plugin60.dsp mp3_plugin.vcproj
