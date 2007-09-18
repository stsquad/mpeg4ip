libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = mad_plugin.la
mad_plugin_la_LDFLAGS = -module
mad_plugin_la_SOURCES = \
	mp3if.cpp \
	mp3if.h \
	mp3_file.cpp \
	mp3_file.h
if HAVE_ID3_TAG
   ID3_LIBS=-lid3 -lid3tag
else
   ID3_LIBS=
endif
mad_plugin_la_LIBADD = \
	-lm $(ID3_LIBS) -lmad \
	$(top_builddir)/player/lib/audio/mp3util/libmp3utils.la \
	$(top_builddir)/lib/fposrec/libmpeg4ip_fposrec.la


INCLUDES = -I$(top_srcdir)/player/src -I$(top_srcdir)/player/lib -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/lib/mp4av -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/lib/utils

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

