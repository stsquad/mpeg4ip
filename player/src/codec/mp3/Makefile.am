libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = mp3_plugin.la
mp3_plugin_la_LDFLAGS = -module
mp3_plugin_la_SOURCES = \
	mp3if.cpp \
	mp3if.h \
	mp3_file.cpp \
	mp3_file.h
if HAVE_ID3_TAG
   ID3_LIBS=-lid3 -lid3tag
else
   ID3_LIBS=
endif
mp3_plugin_la_LIBADD = \
	-lm $(ID3_LIBS) \
	$(top_builddir)/player/lib/audio/mp3/libourmp3.la \
	$(top_builddir)/player/lib/audio/mp3util/libmp3utils.la \
	$(top_builddir)/lib/fposrec/libfposrec.la


noinst_LTLIBRARIES = libmp3.la
libmp3_la_SOURCES = \
	mp3_rtp_bytestream.cpp \
	mp3_rtp_bytestream.h

INCLUDES = -I.. -I../..  -I../../../lib -I../../../lib/audio -I../../../../common/video/mpeg4 -I../../../../include -I../../../../lib -I$(top_builddir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/lib/utils

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = mp3_plugin60.dsp
